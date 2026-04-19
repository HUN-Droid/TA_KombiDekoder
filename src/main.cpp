#include <Arduino.h>
//#include <FlashStorage_STM32.h>
#include "MyEEPROM.h"


/*
    A kombinált vezérlők hardveresen kicsit eltérnek, amik befolyásolhatják a szoftveres lehetőségeket.
    HW 1.0 - több kisebb módosítással készült (smd/dip can transceiver) (3v3 feszstab van/nincs) lábkiosztás és funkciók ugyan azok
    HW 1.1 - kiegészült 2db RS232 porttal, néhány láb módosult, a 12. servo és az i2c ütik egymást (nincs elég IO)
        a könnyebb kezelhetőség végett Kombinált modul vezérlő tipusnévvel fut, ha használjuk az RS232-t
        a debug ha be van kapcsolva, a szomszéd kezelő csak a 2-es porton fut (az 1-en a debug fut)
*/

// BEÁLLÍTÁSOK
// - dupla sorosport kiépítve
//#define SZOMSZED_KEZELO_BOARD

#define useWs
#define useMax
//#define useBovito
//#define useSzomszedKezelo

#define useDebug

#ifdef useDebug
    #define DEBUG_RAILNET
    #define SERVO_DEBUG
    #define PORTBOVITO_DEBUG
#endif

///////////////////

#ifdef SZOMSZED_KEZELO_BOARD
    #define BOARD_HW_VER 0x11
    #ifdef useSzomszedKezelo
        #define BOARD_TYPE 0x51
    #else
        #define BOARD_TYPE 0x50
    #endif
#else
    #define BOARD_HW_VER 0x10
    #define BOARD_TYPE 0x50
#endif

#if !defined(SZOMSZED_KEZELO_BOARD) && defined(useSzomszedKezelo) 
    #error szomszéd kezelő csak SZOMSZED_KEZELO_BOARD-on lehet
#endif

// CV 0..19 - BOARD CV data
#define PORTBOVITO_CV_TIPUS_OFFSET 10

#define SERVO_CV_OFFSET 20
// CV 10.. 12db x 5CV = 69

#define PORTBOVITO_CV_OFFSET 80

#if defined(useWs) || defined(useMax)
    #define JELZO_CV_OFFSET 144
#endif

#include "RailNet.h"

#if defined(useWs) || defined(useMax)
    #include "drivers/jelzo.h"
#endif

#include "drivers/servo.h"

#ifdef useBovito
    #include "drivers/portbovito.h"
#endif

bool railNetFactoryReset() {
    #ifdef DEBUG_RAILNET
        Serial.println("Factory reset");
    #endif
    EEPROM.update(CV_CAN_CIM, 255);
    CAN_CIM = 255;
    EEPROM.update(CV_ERASED_MARKER, CV_ERASED_MARKER_VALUE);
    for(int i=0;i<7;i++) {
        EEPROM.update(CV_MODUL_AZONOSITO+i,' ');
    }

    resetServoCV();

    #ifdef useBovito
        resetPortbovitoCV();
    #endif

    #ifdef useWs
        wsJelzoDriver.factoryReset();
    #endif
    #ifdef useMax
        maxJelzoDriver.factoryReset();
    #endif

    EEPROM.commit();

    return true;
}

bool railNetProcessLongAllitas(uint8_t cim, uint8_t objektum, uint8_t allas) {
    uint8_t group = cim >> 6;

    #ifdef DEBUG_RAILNET
        Serial.print("Állítás cím=");
        Serial.print(cim);
        Serial.print(" objektum=");
        Serial.print(objektum);
        Serial.print(" állás=");
        Serial.print(allas);
        Serial.print(" group=");
        Serial.println(group);
    #endif

    switch(group) {
        case 0: // ws
            #ifdef useWs
                return wsJelzoDriver.allitas(cim & 0b00111111, objektum, allas);
            #else 
                return false;
            #endif
        case 1: // servo
            return servoAllitas(cim & 0b00111111, objektum, allas);
        case 2: // max
            #ifdef useMax
                return maxJelzoDriver.allitas(cim & 0b00111111, objektum, allas);
            #else 
                return false;
            #endif
        default: // 3 - i2c
            #ifdef useBovito
                return portbovitoAllitas(cim & 0b00111111, objektum, allas);
            #else
                return false;
            #endif
    }
}

bool railNetSetParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, bool save, uint8_t value) {
    uint8_t group = cim >> 6;

    switch(group) {
        case 0: // ws
            #ifdef useWs
                return wsJelzoDriver.setParameter(cim & 0b00111111, objektum, dataType, save, value);
            #else 
                return false;
            #endif
        case 1: // servo
            return servoSetParameter(cim & 0b00111111, objektum, dataType, save, value);
        case 2: // max
            #ifdef useMax
                return maxJelzoDriver.setParameter(cim & 0b00111111, objektum, dataType, save, value);
            #else 
                return false;
            #endif
        default: // 3 - i2c            
            return false;
    }
}

bool railNetGetParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, CAN_message_t* CAN_msg) {
    uint8_t group = cim >> 6;

    switch(group) {
        case 0: // ws
            #ifdef useWs
                return wsJelzoDriver.getParameter(cim & 0b00111111, objektum, dataType, CAN_msg);
            #else 
                return false;
            #endif
        case 1: // servo
            return servoGetParameter(cim & 0b00111111, objektum, dataType, CAN_msg);
        case 2: // max
            #ifdef useMax
                return maxJelzoDriver.getParameter(cim & 0b00111111, objektum, dataType, CAN_msg);
            #else 
                return false;
            #endif
        default: // 3 - i2c
            return false;
    }
}

//////////////////////////////////////////////////////////////////////
// Teszt újfajta ws
//////////////////////////////////////////////////////////////////////
/*
static uint8_t encode_table[256][3];
static SPI_HandleTypeDef *hspi_ws;
#define MAX_LEDS 3
static uint8_t spi_buf[MAX_LEDS * 9];
static int led_count = 0;

void WS2811_BuildTable(void) {
    for (int val = 0; val < 256; val++) {
        uint32_t pattern = 0;
        for (int i = 7; i >= 0; i--) {
            uint8_t bit = (val >> i) & 1;
            pattern <<= 3;
            pattern |= bit ? 0b110 : 0b100;
        }
        encode_table[val][0] = (pattern >> 16) & 0xFF;
        encode_table[val][1] = (pattern >>  8) & 0xFF;
        encode_table[val][2] = (pattern >>  0) & 0xFF;
    }
}

void WS2811_Init(SPI_HandleTypeDef *hspi, int count) {
    hspi_ws   = hspi;
    led_count = count;
    WS2811_BuildTable();
}

void WS2811_Show(uint8_t *rgb, int count) {
    if (!spi_buf) return;
    if (count > led_count) count = led_count;

    // minden LED kódolása: GRB sorrend
    for (int i = 0; i < count; i++) {
        uint8_t r = rgb[i*3 + 0];
        uint8_t g = rgb[i*3 + 1];
        uint8_t b = rgb[i*3 + 2];

        memcpy(&spi_buf[i*9 + 0], encode_table[g], 3);
        memcpy(&spi_buf[i*9 + 3], encode_table[r], 3);
        memcpy(&spi_buf[i*9 + 6], encode_table[b], 3);
    }

    HAL_SPI_Transmit_DMA(hspi_ws, spi_buf, count * 9);

/*    // reset idő: min. 50 µs, adunk inkább 80 µs+
    // (SPI vonal addig LOW marad)
    uint32_t start = HAL_GetTick();
    while ( (HAL_GetTick() - start) < 1 ) {
        // 1 ms delay, bőven elég
    }
        */
/*}

#define LED_COUNT 3
uint8_t ledData[LED_COUNT*3];

void setupWs() {    
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // NSS aktív

    // töltsd fel rgb-t (R,G,B)
    WS2811_Init(&hspi2, LED_COUNT);
    //WS2811_Show(ledData, LED_COUNT);
}

uint32_t lastBlinkUpdate = 0;
bool blinkPhase = 0;

void loopWs() {
    if (millis() - lastBlinkUpdate > 1000) {
        lastBlinkUpdate = millis();
        blinkPhase = !blinkPhase;

        if(blinkPhase) {
            ledData[0] = 0;
            ledData[1] = 0;
            ledData[2] = 0;
        }
        else {
            ledData[0] = 255;
            ledData[1] = 0;
            ledData[2] = 0;
        }

        WS2811_Show(ledData, LED_COUNT);
    }
}
*/
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void setup() {    
    #ifdef useDebug
        Serial.begin(115200);
        // auf STM32/ATmega32u4: warten bis USB aktiv (maximal 6sec)
        unsigned long wait=millis()+6000;
        while ( !Serial && (millis()<wait) );        
 
        Serial.print("EEPROM length: ");
        Serial.println(EEPROM.length());

        Serial.println("első adatok:");
        for(int i=0; i<10; i++) {
            Serial.println(EEPROM.read(i));
        }
    #endif
     
    #if defined(useWs) || defined(useMax)
        #ifdef useDebug
            Serial.println("try setup spi");
        #endif
        setupSPI2_DMA();
        #ifdef useDebug
            Serial.println("done spi");
        #endif
        pinMode(LED_BUILTIN, OUTPUT); // TODO: csak tesztnek
        digitalWrite(LED_BUILTIN, LOW);
    #endif

    railNetSetup();

    if(EEPROM.read(CV_ERASED_MARKER) != CV_ERASED_MARKER_VALUE) {
        #ifdef useDebug
            Serial.println("EEPROM törölve, factory reset");
        #endif
        railNetFactoryReset();
    }
    else {
        #ifdef useDebug
            Serial.println("EEPROM valid");
        #endif
    }


    // 1. Órajelek engedélyezése
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    // 2. JTAG letiltása (SWD megmarad)
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    // 3. PB3 kimeneti módra állítása
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/*    // 4. Teszt: PB3 villogtatása
    while (1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
        HAL_Delay(500);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_Delay(500);
    }    */

    delay(300);  // várj 300 ms-ot a táp stabilizálódására
    
    #ifdef useWs
        Serial.println("init ws driver");
        wsJelzoDriver.init();
    #endif
    #ifdef useMax
        Serial.println("init max driver");
        maxJelzoDriver.init();
    #endif
    
    setupServos();

    #ifdef useBovito
        setupPortbovito();
    #endif
    //setupWs();
}

void loop() {
    railNetProcessMessages();
    #ifdef useWs
        wsJelzoController.loop();
    #endif
    #ifdef useMax
        maxJelzoController.loop();
    #endif
    servoLoop();

    //loopWs();
}
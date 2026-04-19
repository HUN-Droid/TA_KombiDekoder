#include "_spi.h"
#include "ws2811.hpp"
#include "max7219.h"
#include <STM32_CAN.h>

#ifndef JELZO_CV_OFFSET
    #error JELZO_CV_OFFSET nincs definiálva
#endif

#define JELZO_CV_COUNT 13
#define CV_JELZO_MODE 0
#define CV_JELZO_ALAPALLAS 1 // 4x
#define CV_JELZO_FENYERO 5 // 4x
#define CV_JELZO_FADE_SPEED 9 // 4x

#define WS_JELZO_PORT_COUNT 3
#define MAX_CHIP_COUNT 1

// a max van elöl
#define MAX_JELZO_CV_OFFSET JELZO_CV_OFFSET
// utána jön a ws
#define WS_JELZO_CV_OFFSET (MAX_JELZO_CV_OFFSET + JELZO_CV_COUNT * MAX_CHIP_COUNT * 8)
// utána a brightness dataset-ek
#define BRIGHTNESS_DATASET_CV_OFFSET (WS_JELZO_CV_OFFSET + JELZO_CV_COUNT * WS_JELZO_PORT_COUNT)

enum class SpiOwner { NONE, WS2811, MAX7219 };
volatile SpiOwner spi_owner = SpiOwner::NONE;

bool spiBusyCheck() {
    return spi2_busy;
}

void maxSpiStart() {
    spi2_busy = true;
    spi_owner = SpiOwner::MAX7219;
}

void wsSpiStart() {
    spi2_busy = true;
    spi_owner = SpiOwner::WS2811;
}

void spiDone() {
    spi2_busy = false;
    spi_owner = SpiOwner::NONE;
}

#include "jelzoController.h"
#include "wsJelzoController.h"
#include "maxJelzoController.h"

BrightnessDataset brightnessDatasets[BRIGHTNESS_DATASET_COUNT];
uint8_t activeBrightnessLevel = 0;
static bool datasetsInitialized = false;

static void setDefaultBrightnessDatasets() {
    for (uint8_t d = 0; d < BRIGHTNESS_DATASET_COUNT; d++) {
        brightnessDatasets[d].level[0].brightness[static_cast<uint8_t>(LedColor::RED)]    = 200;
        brightnessDatasets[d].level[0].brightness[static_cast<uint8_t>(LedColor::YELLOW)] = 180;
        brightnessDatasets[d].level[0].brightness[static_cast<uint8_t>(LedColor::GREEN)]  = 200;
        brightnessDatasets[d].level[0].brightness[static_cast<uint8_t>(LedColor::BLUE)]   = 150;
        brightnessDatasets[d].level[0].brightness[static_cast<uint8_t>(LedColor::WHITE)]  = 220;
        brightnessDatasets[d].level[1].brightness[static_cast<uint8_t>(LedColor::RED)]    = 50;
        brightnessDatasets[d].level[1].brightness[static_cast<uint8_t>(LedColor::YELLOW)] = 45;
        brightnessDatasets[d].level[1].brightness[static_cast<uint8_t>(LedColor::GREEN)]  = 50;
        brightnessDatasets[d].level[1].brightness[static_cast<uint8_t>(LedColor::BLUE)]   = 40;
        brightnessDatasets[d].level[1].brightness[static_cast<uint8_t>(LedColor::WHITE)]  = 55;
    }
}

void initBrightnessDatasets() {
    if (datasetsInitialized) return;
    datasetsInitialized = true;
    if (EEPROM.read(BRIGHTNESS_DATASET_CV_OFFSET) == 255) {
        setDefaultBrightnessDatasets();
        return;
    }
    for (uint8_t d = 0; d < BRIGHTNESS_DATASET_COUNT; d++) {
        for (uint8_t l = 0; l < 2; l++) {
            for (uint8_t c = 0; c < LED_COLOR_COUNT; c++) {
                brightnessDatasets[d].level[l].brightness[c] =
                    EEPROM.read(BRIGHTNESS_DATASET_CV_OFFSET + (d * 2 + l) * LED_COLOR_COUNT + c);
            }
        }
    }
}

void factoryResetBrightnessDatasets() {
    setDefaultBrightnessDatasets();
    for (uint8_t d = 0; d < BRIGHTNESS_DATASET_COUNT; d++) {
        for (uint8_t l = 0; l < 2; l++) {
            for (uint8_t c = 0; c < LED_COLOR_COUNT; c++) {
                EEPROM.update(BRIGHTNESS_DATASET_CV_OFFSET + (d * 2 + l) * LED_COLOR_COUNT + c,
                              brightnessDatasets[d].level[l].brightness[c]);
            }
        }
    }
    datasetsInitialized = true;
}

class JelzoDriver {
    public:
        JelzoDriver(IJelzoController& controller, uint8_t cvOffset) : leds(controller), CV_OFFSET(cvOffset) {}

        void init();

        void factoryReset();

        bool setParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, bool save, uint8_t value);
        bool getParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, CAN_message_t* CAN_msg);

        bool allitas(uint8_t cim, uint8_t objektum, uint8_t allas) {
            if( setJelzoPort(cim, objektum, allas) ) {
                leds.updateDisplay(cim);
                leds.write(1);
                return(true);        
            }            
            return(false);        
        }

        void spiDMADone() {
            leds.onDMADone();
        }

    private:    
        IJelzoController& leds;
        const uint8_t CV_OFFSET;

        jelzo_mask_t get2lencsesMask(uint8_t allas);
        jelzo_mask_t get3lencsesMask(uint8_t allas);
        jelzo_mask_t get4lencsesMagyarMask(uint8_t allas);
        jelzo_mask_t get4lencsesNemetEloMask(uint8_t allas);
        jelzo_mask_t get5lencsesNemetMask(uint8_t allas);

        bool setJelzoPort(uint8_t cim, uint8_t objektum, uint8_t allas);
        void setBits(jelzo_t& jelzo, jelzo_mask_t value, uint8_t offset, uint8_t width);
//        int bInc = 5;
//        bool ledState = false;
//        bool updateNeeded = false;
};

WSJelzoController wsJelzoController;
JelzoDriver wsJelzoDriver(wsJelzoController,WS_JELZO_CV_OFFSET);
MAXJelzoController maxJelzoController;
JelzoDriver maxJelzoDriver(maxJelzoController,MAX_JELZO_CV_OFFSET);

void setFenyeroOsztaly(uint8_t level) {
    if (level > 1) return;
    activeBrightnessLevel = level;
    for (uint8_t i = 0; i < wsJelzoController.getJelzokCount(); i++) {
        wsJelzoController.updateDisplay(i);
    }
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // TODO: csak teszt
        if (spi_owner == SpiOwner::WS2811) {
            wsJelzoDriver.spiDMADone();
        } else if (spi_owner == SpiOwner::MAX7219) {
            maxJelzoDriver.spiDMADone();
        }        
    }
}

jelzo_mask_t JelzoDriver::get2lencsesMask(uint8_t allas) {
    jelzo_mask_t result;
    result.blinkPhaseOn = 0;
    result.blinkPhaseOff = 0;

    switch(allas) {
        case 1: // vörös
            result.mask = 0b01;
            break;
        case 2: // zöld
            result.mask = 0b10;
            break;
        case 4: // szabad a tolatás
            result.mask = 0b10;
            break;
        default: // sötét
            result.mask = 0;
            break;
    }

    return result;
}

jelzo_mask_t JelzoDriver::get3lencsesMask(uint8_t allas) {
    jelzo_mask_t result;
    result.blinkPhaseOn = 0;
    result.blinkPhaseOff = 0;

    switch(allas) {
        case 1: // vörös
            result.mask = 0b001;
            break;
        case 2: // zöld
            result.mask = 0b010;
            break;
        case 4: // szabad a tolatás
            result.mask = 0b010;
            break;
        case 3: // sárga
            result.mask = 0b110;
            break;
        default:
            result.mask =  0;
            break;
    }

    return result;
}

jelzo_mask_t JelzoDriver::get4lencsesNemetEloMask(uint8_t allas) {
    jelzo_mask_t result;
    result.blinkPhaseOn = 0;
    result.blinkPhaseOff = 0;

    switch(allas) {
        case 1: // vörös
            result.mask=0b0011;
            break;
        case 2: // zöld
            result.mask=0b1100;
            break;
        case 3: // sárga
            result.mask=0b0110;
            break;
        default: // sötét
            result.mask=0;
            break;
    }


    return result;
}

jelzo_mask_t JelzoDriver::get4lencsesMagyarMask(uint8_t allas) {
    jelzo_mask_t result;
    result.blinkPhaseOn = 0;
    result.blinkPhaseOff = 0;

    switch(allas) {
        // felső 4bit - következő jelző állása
        // alsó 4 bit - ezen jelző állása
        // lencsék: alsó bit  - vörös
        //                    - zöld
        //                    - felső sárga
        //          felső bit - alsó sárga
        case 1: // vörös
            result.mask =  0b0001;
            break;
        case 0x12: // következő vörös, itt zöld
            result.mask =  0b0100;
            break;
        case 2: // zöld
        case 0x22:  // következő zöld, itt zöld
            result.mask =  0b0010;
            break;
        case 0x32: // következő sárga, itt zöld
            result.mask = 0b0100;
            result.blinkPhaseOn = 0b0100;
            break;
        /*case 0x62: // következő v80, itt zöld
            result.mask = 0b0010;
            result.blinkPhaseOn = 0b0010;
            break;*/
        case 0x13: // következő vörös, itt v40
            result.mask =  0b1100;
            break;
        case 3: // sárga
        case 0x23: // következő zöld, itt v40
            result.mask =  0b1010;
            break;
        case 0x33: // következő v40, itt v40
            result.mask = 0b1100;
            result.blinkPhaseOn = 0b0100;
            break;
        /*case 0x63: // következő v80, itt v40
            result.mask = 0b1010;
            result.blinkPhaseOn = 0b0010;
            break;*/
        // 4 - tolatás
        // 5 - hívó
        default: // sötét
            result.mask =  0;
            break;
    }

    return result;
}

jelzo_mask_t JelzoDriver::get5lencsesNemetMask(uint8_t allas) {
    jelzo_mask_t result;
    result.blinkPhaseOn = 0;
    result.blinkPhaseOff = 0;

    switch(allas) {
        case 1: // vörös
            result.mask=0b00011;
            break;
        case 2: // zöld
            result.mask=0b00100;
            break;
        case 3: // sárga
            result.mask=0b01100;
            break;
        case 4: // szabad a tolatás
            result.mask=0b10010;
            break;
        default: // sötét
            result.mask=0;
            break;
    }

    return result;
}

void JelzoDriver::setBits(jelzo_t& jelzo, jelzo_mask_t value, uint8_t offset, uint8_t width) {
    uint8_t mask = ((1 << width) - 1) << offset;

    jelzo.portData = (jelzo.portData & ~mask) | ((value.mask << offset) & mask);
    jelzo.blinkPhaseOn = (jelzo.blinkPhaseOn & ~mask) | ((value.blinkPhaseOn << offset) & mask);
    jelzo.blinkPhaseOff = (jelzo.blinkPhaseOff & ~mask) | ((value.blinkPhaseOff << offset) & mask);
}

bool JelzoDriver::setJelzoPort(uint8_t cim, uint8_t objektum, uint8_t allas) {
    jelzo_t& jelzo = leds.getJelzo(cim);

    #ifdef useDebug
        Serial.println("Jelző állítás ");
        Serial.println(cim);
        Serial.println(objektum);
        Serial.println(allas);
        Serial.println(static_cast<uint8_t>(jelzo.mode));
    #endif

    if(cim>=leds.getJelzokCount()) return false;

    bool JelzoKiir = 0;    

    switch( jelzo.mode ) {
        case JelzoMode::Bit: { // bitenkénti állítás
            jelzo_mask_t jelzoMask;
            jelzoMask.mask = allas;
            jelzoMask.blinkPhaseOn = 0;
            jelzoMask.blinkPhaseOff = 0;
            setBits(jelzo, jelzoMask, 0, 8);
            JelzoKiir=1;
        }
        break;

        case JelzoMode::UNI_4x2: // 4 db 2 lencsés
            if ( objektum<4 ) {
                auto m = get2lencsesMask(allas);
                setBits(jelzo, m, objektum * 2, 2);
                JelzoKiir=1;
            }
            break;

        case JelzoMode::UNI_2x3_1x2: // 2 db 3 lencsés + 1 db 2 lencsés
            if(objektum==3) {
                auto m = get2lencsesMask(allas);
                setBits(jelzo, m, 6, 2);
                JelzoKiir=1;
            }
            else if( objektum<2 ) {
                auto m = get3lencsesMask(allas);
                setBits(jelzo, m, objektum * 3, 3);
                JelzoKiir=1;
            }
            break;

        case JelzoMode::HV_2x4_elo: // 2 db 4 lencsés német rendszerű (előjelző)
            if( objektum<2 ) {
                auto m = get4lencsesNemetEloMask(allas);
                setBits(jelzo, m, objektum * 4, 4);
                JelzoKiir=1;
            }
            break;

        case JelzoMode::HV_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés német rendszerű
            if( objektum<2 ) {
                if( objektum==0 ) {
                    auto m = get5lencsesNemetMask(allas);
                    setBits(jelzo, m, 0, 5);
                }
                else {
                    auto m = get3lencsesMask(allas);
                    setBits(jelzo, m, 5, 3);
                }
                JelzoKiir=1;
            }
            break;

        case JelzoMode::MAV_2x4: // 2 db 4 lencsés magyar rendszerű
            if( objektum<2 ) {
                auto m = get4lencsesMagyarMask(allas);
                setBits(jelzo, m, objektum * 4, 4);
                JelzoKiir=1;
            }
            break;

        case JelzoMode::MAV_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés magyar rendszerű
            if (objektum < 2) {

                if (objektum == 0) {
                    jelzo_mask_t m;

                    switch (allas) {
                        case 4: // tolatás
                            m = {0b10000, 0, 0};
                            break;

                        case 5: // hívó (villogó fehér)
                            m = {0b10001, 0b10000, 0};  
                            break;

                        default:
                            m = get4lencsesMagyarMask(allas);
                            break;
                    }

                    setBits(jelzo, m, 0, 5);
                }

                if (objektum == 1) {
                    auto m = get3lencsesMask(allas);
                    setBits(jelzo, m, 5, 3);
                }

                JelzoKiir = 1;
            }
            break;

        case JelzoMode::MAV_Fenysorompo_1x2:
            if (objektum < 2) {

                if (objektum == 0) {
                    jelzo_mask_t m;
                    /*
                        lencsék: 2 piros 1 fehér, 2 piros 1 fehér
                    */
                    switch (allas) {
                        case 1: // csukva
                            m = {0b011011, 0b010010, 0b001001};
                            break;

                        case 2: // nyitva
                            m = {0b100100, 0b100100, 0};
                            break;

                        default:
                            m = {0,0,0};
                            break;
                    }

                    setBits(jelzo, m, 0, 6);
                }

                if (objektum == 1) {
                    auto m = get2lencsesMask(allas);
                    setBits(jelzo, m, 6, 2);
                }

                JelzoKiir = 1;
            }
            break;

        case JelzoMode::SK_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés szlovák rendszerű
            if (objektum < 2) {

                if (objektum == 0) {
                    jelzo_mask_t m;

                    switch (allas) {
                        case 4: // tolatás
                            m = {0b10000, 0, 0};
                            break;

                        case 5: // hívó (villogó fehér)
                            m = {0b10001, 0b10000, 0};  
                            break;

                        default:
                            m = get4lencsesMagyarMask(allas);
                            break;
                    }

                    setBits(jelzo, m, 0, 5);
                }

                if (objektum == 1) {
                    jelzo_mask_t m;

                    switch (allas) {
                        case 1: // vörös
                            m = {0b001, 0, 0};
                            break;

                        case 2 : // zöld
                            m = {0b010, 0, 0};
                            break;

                        case 4: // tolatás
                            m = {0b100, 0, 0};
                            break;

                        case 5: // hívó (villogó fehér)
                            m = {0b101, 0b100, 0};  
                            break;

                        default:
                            m = {0,0,0};
                            break;
                    }

                    setBits(jelzo, m, 5, 3);
                }

                JelzoKiir = 1;
            }
            break;

        case JelzoMode::SK_2x4: // 2 db 4 lencsés szlovák rendszerű
            if (objektum < 2) {

                jelzo_mask_t m;

                switch (allas) {
                    case 1: // vörös
                        m = {0b0001, 0, 0};
                        break;

                    case 2 : // zöld
                        m = {0b0010, 0, 0};
                        break;

                    case 3: // sárga
                        m = {0b0110, 0, 0};
                        break;

                    case 4: // tolatás
                        m = {0b1000, 0, 0};
                        break;

                    case 5: // hívó (villogó fehér)
                        m = {0b1001, 0b1000, 0};  
                        break;

                    default:
                        m = {0,0,0};
                        break;
                }

                setBits(jelzo, m, objektum*4, 4);
                JelzoKiir = 1;
            }
            break;

    }

    #ifdef useDebug
        if(JelzoKiir) {
            Serial.println("jelző set done");
        }
    #endif

    return(JelzoKiir);
}

/**
 * @brief Jelzővezérlő funkciók inicializálása
 * 
 */
void JelzoDriver::init() {
    initBrightnessDatasets();
    leds.init();

    uint8_t objCount;
    
    for(uint8_t i=0; i<leds.getJelzokCount(); i++) {
        jelzo_t& jelzo = leds.getJelzo(i);

        uint8_t mode = EEPROM.read(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_MODE);
        if(mode == 255) jelzo.mode = JelzoMode::Bit;
        else jelzo.mode = static_cast<JelzoMode>(mode);
         
        Serial.print("eeprom mode: ");
        Serial.print(mode);
        Serial.print(" jelző mode: ");
        Serial.println(static_cast<uint8_t>(jelzo.mode));
        
        jelzo.portData = 0; // kezdésként sötét
        jelzo.blinkPhaseOn = 0; // és nem villog
        jelzo.blinkPhaseOff = 0; 

        switch( jelzo.mode ) {
            case JelzoMode::UNI_4x2: // 4 db 2 lencsés
                objCount = 4;
            break;
            case JelzoMode::UNI_2x3_1x2: // 2 db 3 lencsés + 1 db 2 lencsés
                objCount = 3;
            break;
            case JelzoMode::HV_2x4_elo: // 2 db 4 lencsés német rendszerű
            case JelzoMode::HV_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés német rendszerű
            case JelzoMode::MAV_2x4: // 2 db 4 lencsés magyar rendszerű
            case JelzoMode::MAV_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés magyar rendszerű
            case JelzoMode::MAV_Fenysorompo_1x2:
            case JelzoMode::SK_1x5_1x3:
            case JelzoMode::SK_2x4:
                objCount = 2;
            break;
            default: // bitenkénti állítás
                objCount = 0;
            break;
        }    
        
        uint8_t dsIdx = EEPROM.read(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_FENYERO);
        jelzo.datasetIndex = (dsIdx < BRIGHTNESS_DATASET_COUNT) ? dsIdx : 0;

        for(uint8_t j=0; j<4; j++) {
            jelzo.alapallas[j] = EEPROM.read(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_ALAPALLAS+j);
            jelzo.fadeSpeed[j] = EEPROM.read(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_FADE_SPEED+j);
            if(j<objCount) {
                setJelzoPort(i,j,jelzo.alapallas[j]);
            }
        }
    }
       
    for(int i=0;i<leds.getJelzokCount();i++) {
        leds.updateDisplay(i);
    }

    //leds.write(true);
}

/**
 * @brief Jelzők beállításainak alaphelyzetbe állítása
 * 
 */
void JelzoDriver::factoryReset() {
    factoryResetBrightnessDatasets();
    for(uint8_t i=0; i<leds.getJelzokCount(); i++) {
        EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_MODE, 0); // bitenkénti állítás
        EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_FENYERO, 0); // dataset index 0
        for(uint8_t j=0; j<4; j++) {
            EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_ALAPALLAS+j, 1); // alapállás vörös
            EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*i + CV_JELZO_FADE_SPEED+j, 120);
        }
    }
}

/**
 * @brief Jelzőkkel kapcsolatos paraméterek beállítása
 * 
 * @param cim 
 * @param objektum 
 * @param dataType CV
 * @param save false: értékek próbálgatása; true: aktuális értékek mentése EEPROM-ba
 * @param value 
 * @return true ha sikerült
 * @return false ha valami hiba volt
 */
bool JelzoDriver::setParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, bool save, uint8_t value) {
    if(cim>=leds.getJelzokCount() || (objektum>=4 && objektum!=0xFF)) return false;

    jelzo_t& jelzo = leds.getJelzo(cim);

    uint8_t oStart = objektum==0xFF ? 0 : objektum;
    uint8_t oEnd = objektum==0xFF ? 3 : objektum;
    
    if(!save) {
        switch(dataType) {
            case CV_JELZO_MODE: 
                jelzo.mode = static_cast<JelzoMode>(value);
                return true;
            case CV_JELZO_ALAPALLAS: 
                for(int i=oStart; i<=oEnd; i++) {
                    jelzo.alapallas[i] = value;
                }
                return true;
            case CV_JELZO_FENYERO:
                jelzo.datasetIndex = (value < BRIGHTNESS_DATASET_COUNT) ? value : 0;
                return true;
            case CV_JELZO_FADE_SPEED: 
                for(int i=oStart; i<=oEnd; i++) {
                    jelzo.fadeSpeed[i] = value;
                }
                return true;
                break;
        }
    } else {
        EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*cim + CV_JELZO_MODE, static_cast<uint8_t>(jelzo.mode));
        EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*cim + CV_JELZO_FENYERO, jelzo.datasetIndex);
        for(int i=oStart; i<=oEnd; i++) {
            EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*cim + CV_JELZO_ALAPALLAS+i, jelzo.alapallas[i]);
            EEPROM.update(CV_OFFSET + JELZO_CV_COUNT*cim + CV_JELZO_FADE_SPEED+i, jelzo.fadeSpeed[i]);
        }
        return true;
    }

    return false;
}

/**
 * @brief Jelzőkkel kapcsolatos paraméterek lekérése
 * 
 * @param cim 
 * @param objektum 
 * @param dataType 
 * @param CAN_msg 
 * @return true 
 * @return false 
 */
bool JelzoDriver::getParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, CAN_message_t* CAN_msg) {
    if(cim>=leds.getJelzokCount() || objektum>=4) return false;

    jelzo_t& jelzo = leds.getJelzo(cim);

    CAN_msg->buf[0] = cim;
    CAN_msg->buf[1] = objektum;
    CAN_msg->buf[2] = dataType;

    switch(dataType) {
        case CV_JELZO_MODE: 
            CAN_msg->buf[3] = static_cast<uint8_t>(jelzo.mode);
            return true;
        case CV_JELZO_ALAPALLAS: 
            CAN_msg->buf[3] = jelzo.alapallas[objektum];
            return true;
        case CV_JELZO_FENYERO:
            CAN_msg->buf[3] = jelzo.datasetIndex;
            return true;
        case CV_JELZO_FADE_SPEED: 
            CAN_msg->buf[3] = jelzo.fadeSpeed[objektum];
            return true;
        break;
    }
    
    return false;
}


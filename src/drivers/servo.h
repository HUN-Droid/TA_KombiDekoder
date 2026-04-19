#pragma once

#include "STM32_ServoDriver.h"

static STM32_ServoDriver ServoDriver;

void STM32_ServoDriver_Handler()
{
    ServoDriver.run();
}


#ifndef SERVO_CV_OFFSET
    #error SERVO_CV_OFFSET nincs definiálva
#endif

#define SERVO_CV_COUNT 5

#define CV_SERVO_CONFIG 0
    #define CV_BIT_USE_POL 0
    #define CV_BIT_AUTO_OFF 1
    #define CV_BIT_POL_DEFAULT_ON 2
#define CV_SERVO_EGYENES_POS 1
#define CV_SERVO_KITERO_POS 2
#define CV_SERVO_SEBESSEG 3
#define CV_SERVO_ALAPALLAS_POS 4


#ifdef SZOMSZED_KEZELO_BOARD
    // TODO: másik alaplap pontos beállításai
    #ifdef useBovito
        #define MAX_SERVO_COUNT 11
    #else
        #define MAX_SERVO_COUNT 12
    #endif

    const uint8_t servoPins[MAX_SERVO_COUNT] = {
        PA0, PA1, PA2, PB1, PB10, PB11,
        PB14, PA8, PA11, PA12, PB4
        #ifndef useBovito
            , PB5
        #endif
    };

    const uint8_t servoPolPins[6] = {
        PA3, PA4, PA5, PA6, PA7, PB0
    };

#else
    #define MAX_SERVO_COUNT 12

    const uint8_t servoPins[MAX_SERVO_COUNT] = {
        PA0, PA1, PA2, PB1, PB10, PB11,
        PB14, PA8, PA11, PA12, PB4, PB5
    };

    const uint8_t servoPolPins[6] = {
        PA3, PA4, PA5, PA6, PA7, PB0
    };
#endif


uint8_t canUsePolAsRelay = 0;


typedef struct {
    bool done;
    bool allowSend;
    uint8_t  cimzett;
    uint8_t  data[3];
} ServoAllitasValaszMsg;

volatile bool needSendValasz = false;
volatile ServoAllitasValaszMsg servoAllitasValaszMsg[MAX_SERVO_COUNT];

bool servoStarting = false;
uint8_t servoStartIndex = 0;
unsigned long nextServoStartTime;
const unsigned long servo_start_time = 500; // ms

// ISR függvény!!!
void notifyServoReached(uint8_t servoIndex) {
    #ifdef SERVO_DEBUG
        Serial.print(servoIndex);
        Serial.println(" servo célba ért");
    #endif

    servoAllitasValaszMsg[servoIndex].done = true;
    needSendValasz = true;
}

void servoLoop() {
    if(needSendValasz) {
        for(int i=0; i<MAX_SERVO_COUNT; i++) {
            if(servoAllitasValaszMsg[i].done) {
                servoAllitasValaszMsg[i].done = false;
                if(servoAllitasValaszMsg[i].allowSend) {
                    servoAllitasValaszMsg[i].allowSend = false;
                    for(int j=0; j<3; j++) {
                        CAN_TX_msg.buf[j] = servoAllitasValaszMsg[i].data[j];
                    }
                    CANsend(servoAllitasValaszMsg[i].cimzett,7,3);
                }
            }
        }
        needSendValasz = false;
    }

    if(servoStarting && ( nextServoStartTime <= millis()) ) {
        while (servoStartIndex<MAX_SERVO_COUNT && !ServoDriver.isValid(servoStartIndex)) {
            servoStartIndex++;
        }
        
        if(ServoDriver.isValid(servoStartIndex)) {
            #ifdef SERVO_DEBUG
                Serial.print("Move servo to start position idx=");
                Serial.println(servoStartIndex);
            #endif            
            ServoDriver.setEnabled(servoStartIndex);
            servoStartIndex++;
            nextServoStartTime = millis() + servo_start_time;
        }

        if(servoStartIndex>=MAX_SERVO_COUNT) {
            // start vége
            #ifdef SERVO_DEBUG
                Serial.println("Move servo to start position DONE");
            #endif            
            servoStarting = false;
        } 
    }
}

void setupServos() {
    
    /*setupIsrServo();
    
    // Például 3 szervó inicializálása
    setupServo(0, PB10, 1000, 2000, 3000);  // PB0 - 1ms -> 2ms 3s alatt
    setupServo(1, PB11, 1500, 1000, 2000);  // PB1 - 1.5ms -> 1ms 2s alatt
    setupServo(2, PB14, 1200, 1800, 4000); // PB10 - 1.2ms -> 1.8ms 4s alatt
*/
    for(uint8_t i = 0; i<MAX_SERVO_COUNT; i++) {
        uint8_t conf = EEPROM.read(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_CONFIG);
        bool usePol = false;
        uint8_t polPin = i<6 ? servoPolPins[i] : STM32_WRONG_PIN;

        if(i<6) {
            usePol = conf & (1 << CV_BIT_USE_POL);
            if(!usePol) canUsePolAsRelay |= (1<<i);
            pinMode(polPin, OUTPUT);
            digitalWrite(polPin, conf & (1 << CV_BIT_POL_DEFAULT_ON) );
        }
        
        uint8_t si = ServoDriver.setupServo(servoPins[i], conf,
            //50, 0, 180, 0);
            EEPROM.read(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_SEBESSEG),
            EEPROM.read(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_EGYENES_POS),
            EEPROM.read(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_KITERO_POS),
            EEPROM.read(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_ALAPALLAS_POS),
            usePol ? polPin : STM32_WRONG_PIN
        );

        #ifdef DEBUG_RAILNET
            Serial.print("Setup servo i=");
            Serial.println(i);
        
            Serial.print("new servoIndex=");
            Serial.println(si);
        #endif
    }

    //Serial.println("Try setup timer");

    // Relék start helyzetbe állítása
    for(uint8_t objektum=0; objektum<6; objektum++) {
        if(canUsePolAsRelay & (1<<objektum)) {          
            uint8_t conf = EEPROM.read(SERVO_CV_OFFSET + SERVO_CV_COUNT*objektum + CV_SERVO_CONFIG);  
            digitalWrite(servoPolPins[objektum], conf & (1<<CV_BIT_POL_DEFAULT_ON) );
        }
    }
        
    ServoDriver.start();
    servoStarting = true;
    servoStartIndex = 0;
    nextServoStartTime = millis();

    //Serial.println("Start timer skipped");
}

void resetServoCV() {
    for(uint8_t i = 0; i<MAX_SERVO_COUNT; i++) {
        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_CONFIG, 255);
        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_SEBESSEG, 50);
        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_EGYENES_POS, 90);
        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_KITERO_POS, 95);
        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_ALAPALLAS_POS, 90);
    }
}

bool servoSetParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, bool save, uint8_t value) {
    // servo
    if(cim==0) {
        if((objektum>=MAX_SERVO_COUNT && objektum!=0xFF) || dataType>=SERVO_CV_COUNT) {
            #ifdef SERVO_DEBUG
                Serial.println("Hiba: objektum>=MAX_SERVO_COUNT || dataType>=SERVO_CV_COUNT");
            #endif
            return(false);
        }    

        uint8_t oStart = objektum==0xFF ? 0 : objektum;
        uint8_t oEnd = objektum==0xFF ? ServoDriver.MAX_SERVOS-1 : objektum;
    
        if(!save) {
            switch(dataType) {
                case CV_SERVO_EGYENES_POS:
                    for(int i=oStart; i<=oEnd; i++) {
                        ServoDriver.setEgyenesAllasPos(i, value);
                    }
                    return true;
                case CV_SERVO_KITERO_POS:
                    for(int i=oStart; i<=oEnd; i++) {
                        ServoDriver.setKiteroAllasPos(i, value);
                    }
                    return true;
                case CV_SERVO_SEBESSEG:
                    for(int i=oStart; i<=oEnd; i++) {
                        ServoDriver.setSpeed(i, value);
                    }
                    return true;
            }
        } else {
            #ifdef SERVO_DEBUG
                Serial.println("Servo adatok mentése EEPROM-ba");
            #endif

            switch(dataType) {
                case CV_SERVO_CONFIG:
                    for(int i=oStart; i<=oEnd; i++) {
                        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_CONFIG, value);
                    }
                    return true;
                case CV_SERVO_ALAPALLAS_POS:
                    for(int i=oStart; i<=oEnd; i++) {
                        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_ALAPALLAS_POS, value);
                    }
                    return true;
                case 0b01111111:
                    for(int i=oStart; i<=oEnd; i++) {
                        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_SEBESSEG, ServoDriver.getSpeed(i));
                        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_EGYENES_POS, ServoDriver.getEgyenesAllasPos(i));
                        EEPROM.update(SERVO_CV_OFFSET + SERVO_CV_COUNT*i + CV_SERVO_KITERO_POS, ServoDriver.getKiteroAllasPos(i));
                    }
                    return true;
            }
            
        }
    }
    
    return false;
}

bool servoGetParameter(uint8_t cim, uint8_t objektum, uint8_t dataType, CAN_message_t* CAN_msg) {
    // servo
    if(cim==0) {
        if(objektum>=MAX_SERVO_COUNT || dataType>=SERVO_CV_COUNT) {
            #ifdef SERVO_DEBUG
                Serial.println("Hiba: objektum>=MAX_SERVO_COUNT || dataType>=SERVO_CV_COUNT");
            #endif
            return(false);
        }    

        CAN_msg->buf[0] = 64;
        CAN_msg->buf[1] = objektum;
        CAN_msg->buf[2] = dataType;

        switch(dataType) {
            case CV_SERVO_EGYENES_POS:
                CAN_msg->buf[3] = ServoDriver.getEgyenesAllasPos(objektum);
                return true;
            case CV_SERVO_KITERO_POS:
                CAN_msg->buf[3] = ServoDriver.getKiteroAllasPos(objektum);
                return true; 
            case CV_SERVO_SEBESSEG:
                CAN_msg->buf[3] = ServoDriver.getSpeed(objektum);
                return true;
        }
    }
    
    return false;
}

void railNetBeforeProcessLongAllitas(uint8_t felado, uint8_t* data) {
    if(data[0]==64) {
        uint8_t servoIndex = data[1];
        #ifdef SERVO_DEBUG
            Serial.print("Új állás mentése későbbre. címzett=");
            Serial.print(felado);
            Serial.print(" servoIndex=");
            Serial.println(servoIndex);
        #endif
        servoAllitasValaszMsg[servoIndex].cimzett = felado;
        for(uint8_t i=0; i<3; i++) {
            servoAllitasValaszMsg[servoIndex].data[i] = data[i];
        }
    }
}

bool servoAllitas(uint8_t cim, uint8_t objektum, uint8_t allas) {
    #ifdef SERVO_DEBUG
        Serial.print("Servo állítás cím=");
        Serial.println(cim);
    #endif

    // servo
    if(cim==0) {
        if(objektum>=MAX_SERVO_COUNT || allas>1) {
            #ifdef SERVO_DEBUG
                Serial.println("Hiba: objektum>=MAX_SERVO_COUNT || allas>1");
            #endif
            return(false);
        }
        servoAllitasValaszMsg[objektum].done = false;

        if(ServoDriver.getAllas(objektum)==allas) {
            // aktuális állás = vezérelt állás -> mehet a válasz
            return true;
        }

        servoAllitasValaszMsg[objektum].allowSend = true;

        // innen küldünk egy ACK-t
        for(int j=0; j<3; j++) {
            CAN_TX_msg.buf[j] = servoAllitasValaszMsg[objektum].data[j];
        }
        CAN_TX_msg.buf[3] = 1; // vezérlés indul
        
        CANsend(servoAllitasValaszMsg[objektum].cimzett,7,4);

        // indítás
        ServoDriver.setAllas(objektum, allas);
        // return false, mert a választ a célraérkezéskor küldjük
    }

    // pol
    else if(cim==2) {
        if(objektum>=6 || allas>1) {
            return(false);
        }
        if(canUsePolAsRelay & (1<<objektum)) {
            digitalWrite(servoPolPins[objektum],allas==0?LOW:HIGH);
            return true;
        }
    }

    return false;
}
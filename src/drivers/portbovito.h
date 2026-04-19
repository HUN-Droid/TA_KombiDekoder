#pragma once

#include "_i2c.h"

// MCP23017 regiszterek
#define MCP_IODIRA   0x00
#define MCP_IODIRB   0x01
#define MCP_PULLUPA  0x0C
#define MCP_PULLUPB  0x0D
#define MCP_GPIOA    0x12
#define MCP_GPIOB    0x13

// Regiszterek
uint8_t letezik = 0;

// kártyatípusok
char kartyaTipus[16];
                   //    0,1 - I2C=0
                   //    2,3 - I2C=1
                   //    ...
                   // Értékek:
const char tpValto         = 1;
const char tpValtoBemenet  = 2;
const char tpIdozitett     = 3;
const char tpRele          = 4;
const char tpBemenet       = 5;
const char tpForditokorong = 6;

// portok értékei
char PORT[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                   //    0,1 - I2C=0
                   //    2,3 - I2C=1
                   //    ...

#ifndef PORTBOVITO_CV_OFFSET
    #error PORTBOVITO_CV_OFFSET nincs definiálva
#endif
#ifndef PORTBOVITO_CV_TIPUS_OFFSET
    #error PORTBOVITO_CV_TIPUS_OFFSET nincs definiálva
#endif

#define PORTBOVITO_CV_COUNT 8


// bővítő portra írás
void write_23017(uint8_t cim, uint8_t address, uint8_t adat) {
    uint8_t c;

    if( letezik & (1<<cim) ) {
        c=(0x20+cim);
        
        uint8_t data[2];
        data[0] = address;
        data[1] = adat;

        HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(&hi2c1, c << 1, data, 2, 100);
        #ifdef PORTBOVITO_DEBUG
            Serial.printf("HAL_I2C_Master_Transmit returned: %d\r\n", res);
        #endif

        if( res != HAL_OK ) {
            #ifdef PORTBOVITO_DEBUG
                Serial.printf("0x%X címen mcp23017 nem elérhető\r\n",c);
            #endif
        }
    }

    else {
        #ifdef PORTBOVITO_DEBUG
            Serial.printf("nincs ilyen cím (0x%X);\r\n",cim);
        #endif
    }
}

void kiir(char cim) {
    if( cim & 1 ) {
        // Páratlan - PortB
        write_23017(cim/2, MCP_GPIOB, PORT[cim]);
        //if( kartyaTipus[cim]==tpRele ) write_23017(cim/2, 0x01, 0x00);

    }
    else {
        // Páros - PortA
        write_23017(cim/2, MCP_GPIOA, PORT[cim]);
        //if( kartyaTipus[cim]==tpRele ) write_23017(cim/2, 0x00, 0x00);
    }
}

void output(uint8_t cim, uint8_t pin, uint8_t state) {
    if (state == 0) {
        PORT[cim] &= (~(1<<pin));
    }
    else {
        PORT[cim] |= (1<<pin);
    }
    kiir(cim);
}

void loadPortbovitoConfig() {
    for(uint8_t a=0;a<8;a++) {
        uint8_t j = EEPROM.read(PORTBOVITO_CV_TIPUS_OFFSET+a);
        if( j != 0 && j != 0xFF ) {
            uint8_t m = 1;
            m = m<<a;
            letezik |= m;
        }
        kartyaTipus[a*2] = j & 0b00001111;
        kartyaTipus[a*2+1] = j >> 4;
    }

    #ifdef DEBUG_RAILNET
        Serial.printf("letezik = %u\r\n",letezik);
    #endif
}

/**
 * Frissíti a letezik bitmaskot a ténylegesen elérhető MCP23017-ek szerint
 */
void check_mcp_devices(void) {
    for (uint8_t cim = 0; cim < 8; cim++) {
        uint8_t devAddr = (0x20 + cim) << 1;  // 7-bites cím (0x20–0x27) balra tolva

        HAL_StatusTypeDef res = HAL_I2C_IsDeviceReady(&hi2c1, devAddr, 3, 50);

        if (res == HAL_OK) {
            // Eszköz elérhető
            letezik |= (1U << cim);
            #ifdef PORTBOVITO_DEBUG
                Serial.printf("MCP23017 van a 0x%02X címen\r\n", (0x20 + cim));
            #endif
        } else {            
            // Eszköz nem elérhető → bit törlése
            letezik &= ~(1U << cim);
            #ifdef PORTBOVITO_DEBUG
                Serial.printf("MCP23017 nincs a 0x%02X címen (kód = %d)\r\n", (0x20 + cim), res);
            #endif
        }
    }

    #ifdef DEBUG_RAILNET
        Serial.printf("letezik = %u\r\n",letezik);
    #endif    
}

void setupPortbovito() {
    setupI2C1();
    loadPortbovitoConfig();

    check_mcp_devices();

    // mcp inicializálás
    for(uint8_t i=0; i<8; i++) {
        if( letezik & (1<<i) ) {
            write_23017(i, MCP_IODIRA, 0x00);
            write_23017(i, MCP_IODIRB, 0x00);
            // TODO: kezdőértékek
        }
    }
}

void resetPortbovitoCV() {
    for(uint8_t a=0;a<8;a++) {
        EEPROM.update(PORTBOVITO_CV_TIPUS_OFFSET+a, tpRele); // TODO: mi legyen a kezdőérték?
    }
}

bool portbovitoAllitas(uint8_t cim, uint8_t objektum, uint8_t allas) {
    #ifdef PORTBOVITO_DEBUG
        Serial.printf("Portbővítő állítás cím=%u obj=%u allas=%u\r\n", cim, objektum, allas);        
    #endif

    if(cim<16 && objektum<8) {
        // TODO: mindent reléként kezelünk
        //switch( kartyaTipus[cim] ) {
            // - Relé
        //    case tpRele:
                // folyamatos, különálló
                output(cim,objektum,allas);
                return true;
        //    break;
        //}
    }

    return(false);
}
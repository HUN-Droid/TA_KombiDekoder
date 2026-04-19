#pragma once

#ifndef STM32_Servo_DRIVER_H
#define STM32_Servo_DRIVER_H

#include <Arduino.h>

#define STM32_WRONG_PIN         255

#define MIN_PULSE_WIDTH         800       // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH         2450      // the longest pulse sent to a servo 
#define REFRESH_INTERVAL        20000     // minumim time to refresh servos in microseconds 

#define MIN_STEP_INTERVAL 200//0   // leggyorsabb: 2 ms (500 lépés/másodperc)
#define MAX_STEP_INTERVAL 3000//0  // leglassabb: 20 ms (50 lépés/másodperc)

// Use 10 microsecs timer, just fine enough to control Servo, normally requiring pulse width (PWM) 500-2000us in 20ms.
#define TIMER_INTERVAL_MICRO        10

#define SERVO_USE_POL 1
#define SERVO_AUTO_OFF 2

extern void STM32_ServoDriver_Handler() __attribute__ ((weak));
extern void notifyServoReached(uint8_t servoIndex) __attribute__ ((weak));

class STM32_ServoDriver {
    public:
        // maximum number of servos
        const static int MAX_SERVOS = 16;

        // constructor
        STM32_ServoDriver();

        // handle servos
        void run();

        // Bind servo to the timer and pin, return servoIndex
        int8_t setupServo(uint8_t pin, uint8_t config, uint8_t speed,
            uint8_t egyenesPos, uint8_t kiteroPos, uint8_t startPos,
            uint8_t polPin = STM32_WRONG_PIN);

        // servo vezérlése állás szerint
        bool setAllas(uint8_t servoIndex, bool kitero);
        bool getAllas(uint8_t servoIndex);
        
        bool setSpeed(uint8_t servoIndex, uint8_t speed);
        uint8_t getSpeed(uint8_t servoIndex);

        bool setEgyenesAllasPos(uint8_t servoIndex, uint8_t pos);
        uint8_t getEgyenesAllasPos(uint8_t servoIndex);

        bool setKiteroAllasPos(uint8_t servoIndex, uint8_t pos);
        uint8_t getKiteroAllasPos(uint8_t servoIndex);

        // start pwm generator
        void start() {
            #ifdef SERVO_DEUG
                Serial.println("Start servo timer");
            #endif
            // Timer indítása
            timer->resume();
        }

        bool isValid(uint8_t servoIndex) {
            if (servoIndex >= MAX_SERVOS)
                return false;

            return servo[servoIndex].pin != STM32_WRONG_PIN;
        }

        void setEnabled(uint8_t servoIndex) {
            if (servoIndex < MAX_SERVOS) {
                servo[servoIndex].enabled = true;
            }
        }

    private:

        void init() {
            for (uint8_t servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++) {
                memset((void*) &servo[servoIndex], 0, sizeof (servo_t));
                servo[servoIndex].targetCount = 0;
                servo[servoIndex].enabled = false;
                // Intentional bad pin
                servo[servoIndex].pin = STM32_WRONG_PIN;
                servo[servoIndex].polPin = STM32_WRONG_PIN;
            }

            numServos   = 0;

            // Init timerCount
            timerCount  = 1;
            tickCounter = 0;

            // Timer példány létrehozása
            timer = new HardwareTimer(TIM2);
            // Timer leállítása, ha futna
            timer->pause();
            // Timer beállítása adott frekvenciára
            timer->setOverflow(TIMER_INTERVAL_MICRO, MICROSEC_FORMAT);
            // Callback függvény, ami megszakításkor hívódik
            timer->attachInterrupt(STM32_ServoDriver_Handler);
        }

        // find the first available slot
        int findFirstFreeSlot();

        typedef struct {
            uint8_t  pin;              // pin servo connected to
            uint8_t  polPin;           // polarizáció GPIO pinje

            bool enabled;              // true if enabled
            bool kiteroAllas;          // aktuális vezérelt állás

            uint16_t  targetCount;     // cél pozíció In tick
            uint16_t  currentCount;    // aktuális pozíció In tick
            uint16_t  startCount;      // mozgatás kezdőpontja
            uint16_t  halfway;         // félúti pozíció
                                    
            uint32_t lastStepTick;
            uint32_t stepIntervalTick;

            uint8_t speed;
            uint8_t targetPosition;
            uint8_t egyenesAllasPos;
            uint8_t kiteroAllasPos;
            uint8_t config;             // beállítások: polaizálás, jel autoOff

            bool halfwayTriggered;
            bool reachedCallbackCalled;
            bool movingToDiverted; // kitérő felé mozog
        } servo_t;

        void calcHalfway(uint8_t servoIndex);

        volatile servo_t servo[MAX_SERVOS];

        // actual number of servos in use (-1 means uninitialized)
        volatile int8_t numServos;

        // timerCount starts at 1, and counting up to (REFRESH_INTERVAL / TIMER_INTERVAL_MICRO) = (20000 / 10) = 2000
        // then reset to 1. Use this to calculate when to turn ON / OFF pulse to servo
        // For example, servo1 uses pulse width 1000us => turned ON when timerCount = 1, turned OFF when timerCount = 1000 / TIMER_INTERVAL_MICRO = 100
        volatile unsigned long timerCount;

        volatile uint32_t tickCounter;        
        HardwareTimer *timer = nullptr;
};

#endif

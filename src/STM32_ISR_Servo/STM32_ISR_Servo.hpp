/****************************************************************************************************************************
 STM32_ISR_Servo.hpp
For STM32F/L/H/G/WB/MP1 boards with stm32duino Arduino_Core_STM32 core
Written by Khoi Hoang

Built by Khoi Hoang https://github.com/khoih-prog/STM32_ISR_Servo
Licensed under MIT license

Based on SimpleTimer - A timer library for Arduino.
Author: mromani@ottotecnica.com
Copyright (c) 2010 OTTOTECNICA Italy

Based on BlynkTimer.h
Author: Volodymyr Shymanskyy

Version: 1.1.0

Version Modified By   Date      Comments
------- -----------  ---------- -----------
1.0.0   K Hoang      15/08/2021 Initial coding for STM32F/L/H/G/WB/MP1
1.1.0   K Hoang      06/03/2022 Convert to `h-only` style. Optimize code by using passing by `reference`
*****************************************************************************************************************************/

#pragma once

#ifndef STM32_ISR_Servo_HPP
#define STM32_ISR_Servo_HPP

#if !defined(STM32_ISR_SERVO_VERSION)
#define STM32_ISR_SERVO_VERSION             "STM32_ISR_Servo v1.1.0"

#define STM32_ISR_SERVO_VERSION_MAJOR       1
#define STM32_ISR_SERVO_VERSION_MINOR       1
#define STM32_ISR_SERVO_VERSION_PATCH       0

#define STM32_ISR_SERVO_VERSION_INT         1001000

#endif

#include <stddef.h>

#include <inttypes.h>

#if defined(ARDUINO)
#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif
#endif

#include "STM32_ISR_Servo_Debug.h"
#include "STM32_FastTimerInterrupt.h"

#define STM32_MAX_PIN           NUM_DIGITAL_PINS
#define STM32_WRONG_PIN         255

// From Servo.h - Copyright (c) 2009 Michael Margolis.  All right reserved.

#define MIN_PULSE_WIDTH         800       // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH         2450      // the longest pulse sent to a servo 
#define DEFAULT_PULSE_WIDTH     1500      // default pulse width when servo is attached
#define REFRESH_INTERVAL        20000     // minumim time to refresh servos in microseconds 

#define MIN_STEP_INTERVAL 200//0   // leggyorsabb: 2 ms (500 lépés/másodperc)
#define MAX_STEP_INTERVAL 2000//0  // leglassabb: 20 ms (50 lépés/másodperc)

// Use 10 microsecs timer, just fine enough to control Servo, normally requiring pulse width (PWM) 500-2000us in 20ms.
#define TIMER_INTERVAL_MICRO        10

extern void STM32_ISR_Servo_Handler();

class STM32_ISR_Servo
{

    public:
        // maximum number of servos
        const static int MAX_SERVOS = 16;

        // constructor
        STM32_ISR_Servo();

        // destructor
        ~STM32_ISR_Servo()
        {
            /*if (STM32_ITimer)
            {
                STM32_ITimer->detachInterrupt();
                delete STM32_ITimer;
            }*/
        }

        void run();

        // useTimer select which timer (0-3) of STM32 to use for Servos
        //Return true if timerN0 in range
        bool useTimer(TIM_TypeDef* timerNo)
        {
            //_timerNo = timerNo;
            return true;
        }

        // Bind servo to the timer and pin, return servoIndex
        int8_t setupServo(uint8_t pin, bool autoOff, uint8_t speed,
            uint8_t egyenesPos, uint8_t kiteroPos, uint8_t startPos,
            uint8_t polPin = STM32_WRONG_PIN);
            //uint16_t min = MIN_PULSE_WIDTH, uint16_t max = MAX_PULSE_WIDTH);

        // setPosition will set servo to position in degrees
        // by using PWM, turn HIGH 'duration' microseconds within REFRESH_INTERVAL (20000us)
        // returns true on success or -1 on wrong servoIndex
        bool setPosition(uint8_t servoIndex, const float& position);

        bool setAllas(uint8_t servoIndex, bool kitero);

        // returns last position in degrees if success, or -1 on wrong servoIndex
        float getPosition(uint8_t servoIndex);

        bool setSpeed(uint8_t servoIndex, uint8_t speed);

        // setPulseWidth will set servo PWM Pulse Width in microseconds, correcponding to certain position in degrees
        // by using PWM, turn HIGH 'pulseWidth' microseconds within REFRESH_INTERVAL (20000us)
        // min and max for each individual servo are enforced
        // returns true on success or -1 on wrong servoIndex
        bool setPulseWidth(uint8_t servoIndex, uint16_t& pulseWidth);

        // returns pulseWidth in microsecs (within min/max range) if success, or 0 on wrong servoIndex
        uint16_t getPulseWidth(uint8_t servoIndex);

        // destroy the specified servo
        void deleteServo(uint8_t servoIndex);

        // returns true if the specified servo is enabled
        bool isEnabled(uint8_t servoIndex);

        // enables the specified servo
        bool enable(uint8_t servoIndex);

        // disables the specified servo
        bool disable(uint8_t servoIndex);

        // enables all servos
        void enableAll();

        // disables all servos
        void disableAll();

        // enables the specified servo if it's currently disabled,
        // and vice-versa
        bool toggle(uint8_t servoIndex);

        // returns the number of used servos
        int8_t getNumServos();

        // returns the number of available servos
        int8_t getNumAvailableServos() 
        {
            if (numServos <= 0)
                return MAX_SERVOS;
            else 
                return MAX_SERVOS - numServos;
        };

        void start() {
            Serial.println("init timer");

            // Timer példány létrehozása
            timer = new HardwareTimer(TIM2);
            Serial.println("timer created");

            // Timer leállítása, ha futna
            timer->pause();
            Serial.println("timer paused");

            // Timer beállítása adott frekvenciára
            timer->setOverflow(TIMER_INTERVAL_MICRO, MICROSEC_FORMAT);
            Serial.println("overflow set");
            
            
            // Callback függvény, ami megszakításkor hívódik
            timer->attachInterrupt(STM32_ISR_Servo_Handler);
            Serial.println("int attached");

            // Timer indítása
            timer->resume();            
            Serial.println("timer resumed");
        }
    private:

        void init(bool autoStartTimer = true)
        {
            //STM32_ITimer = new STM32FastTimer(_timerNo);

            // Interval in microsecs
            /*if ( STM32_ITimer && STM32_ITimer->attachInterruptInterval(TIMER_INTERVAL_MICRO, (stm32_timer_callback) STM32_ISR_Servo_Handler ) )
            {
                ISR_SERVO_LOGERROR("Starting  ITimer OK");
            }
            else
            {
                ISR_SERVO_LOGERROR("Fail setup STM32_ITimer");      
            }
*/
            for (uint8_t servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++)
            {
                memset((void*) &servo[servoIndex], 0, sizeof (servo_t));
                servo[servoIndex].targetCount    = 0;
                servo[servoIndex].enabled  = false;
                // Intentional bad pin
                servo[servoIndex].pin      = STM32_WRONG_PIN;
            }

            numServos   = 0;

            // Init timerCount
            timerCount  = 1;
            timerMicros = 0;
            tickCounter = 0;
        }

        // find the first available slot
        int findFirstFreeSlot();

        typedef struct
        {
            uint8_t       pin;                  // pin servo connected to
            unsigned int targetCount;                // In microsecs
            unsigned int currentCount;                // In microsecs
            float         position;             // In degrees
            bool          enabled;              // true if enabled
            //uint16_t      min;
            //uint16_t      max;
            unsigned int egyenesAllasCount; // In microsecs
            unsigned int kiteroAllasCount; // In microsecs

            uint8_t speed; 
            unsigned int stepIntervalMicros;  // Mennyi idő teljen el 1 count lépéshez (sebesség)
            unsigned long lastStepMicros;     // Mikor történt utoljára lépés

            unsigned int startCount;              // mozgatás kezdőpontja
            unsigned int halfway;                 // félúti pozíció
            bool     halfwayTriggered;        // hogy egyszer triggereltük-e már
            uint8_t  polPin;              // polarizáció GPIO pinje

            bool reachedCallbackCalled;
            bool autoOff;


            uint32_t lastStepTick;
            uint32_t stepIntervalTick;
        
        } servo_t;

        volatile servo_t servo[MAX_SERVOS];

        // actual number of servos in use (-1 means uninitialized)
        volatile int8_t numServos;

        // timerCount starts at 1, and counting up to (REFRESH_INTERVAL / TIMER_INTERVAL_MICRO) = (20000 / 10) = 2000
        // then reset to 1. Use this to calculate when to turn ON / OFF pulse to servo
        // For example, servo1 uses pulse width 1000us => turned ON when timerCount = 1, turned OFF when timerCount = 1000 / TIMER_INTERVAL_MICRO = 100
        volatile unsigned long timerCount;
        volatile uint32_t tickCounter;

        volatile unsigned long timerMicros;

        // For STM32 timer
        //TIM_TypeDef*      _timerNo;
        //STM32FastTimer*   STM32_ITimer;
        HardwareTimer *timer = nullptr;
};

#endif    // STM32_ISR_Servo_HPP

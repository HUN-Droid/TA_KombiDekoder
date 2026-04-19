#ifndef driverSTM32_H
#define driverSTM32_H

// STM32F1 HW-specific Functions using STM32 Arduino HAL
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <Servo.h>
#include "drivers.h"

// Timer and channel for PWM (Timer4 used for both stepper and servo in original)
// We assume TIM4_CH2 is used for servo PWM generation
#define SERVO_TIMER      TIM4
#define SERVO_CHANNEL    TIM_CHANNEL_2
#define SERVO_IRQ        TIM4_IRQn
#define SERVO_HANDLER    TIM4_IRQHandler

// Time conversion
#define TICS_PER_MICROSECOND 2  // Assuming timer clock is 2 MHz (0.5us per tick)

static HardwareTimer *servoTimer = nullptr;

void ISR_Servo();

void seizeTimerAS() {
    static bool timerInitialized = false;
    if (timerInitialized) return;

    // Configure Timer4 with prescaler to get 0.5us ticks (assuming 72MHz clock)
    servoTimer = new HardwareTimer(SERVO_TIMER);
    servoTimer->setPrescaleFactor(36); // 72MHz / 36 = 2MHz → 0.5us
    servoTimer->setOverflow(20000, MICROSEC_FORMAT); // 20ms full period
    servoTimer->setMode(SERVO_CHANNEL, TIMER_OUTPUT_COMPARE);
    servoTimer->setCaptureCompare(SERVO_CHANNEL, FIRST_PULSE, MICROSEC_COMPARE_FORMAT);
    servoTimer->attachInterrupt(SERVO_CHANNEL, ISR_Servo);
    servoTimer->resume();

    pinMode(PB7, OUTPUT); // Debug pin if needed

    timerInitialized = true;
}

void enableServoIsrAS() {
    if (!servoTimer) return;
    servoTimer->attachInterrupt(SERVO_CHANNEL, ISR_Servo);
}

void setServoCmpAS(uint16_t cmpValue) {
    if (!servoTimer) return;
    servoTimer->setCaptureCompare(SERVO_CHANNEL, cmpValue, TICK_COMPARE_FORMAT);
}

#endif

#endif
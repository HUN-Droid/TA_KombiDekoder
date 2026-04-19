/****************************************************************************************************************************
  STM32_ISR_Servo.cpp
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

#include "STM32_ISR_Servo.h"
#include <string.h>

#ifndef ISR_SERVO_DEBUG
  #define ISR_SERVO_DEBUG               1
#endif

#define DEFAULT_STM32_TIMER_NO          TIMER_SERVO       // TIM7 for many boards

static STM32_ISR_Servo STM32_ISR_Servos;  // create servo object to control up to 16 servos

extern void notifyServoReached(uint8_t servoIndex) __attribute__ ((weak));

void STM32_ISR_Servo_Handler()
{
  STM32_ISR_Servos.run();
}

STM32_ISR_Servo::STM32_ISR_Servo()
  : numServos (-1)//, _timerNo(DEFAULT_STM32_TIMER_NO)
{
}

static uint8_t currentServoIndex = 0;     // aktuális szervó index slice-hoz

void STM32_ISR_Servo::run() {
  tickCounter++;

  if (timerCount == 1) {
      // --- REFRESH ciklus eleje: minden szervót HIGH-ba egyszerre ---
      for (int i = 0; i < MAX_SERVOS; i++) {
          if (servo[i].enabled) {
              digitalWrite(servo[i].pin, HIGH);
          }
      }
  }

  // --- LOW váltások ---
  for (int i = 0; i < MAX_SERVOS; i++) {
      if (servo[i].enabled && timerCount == servo[i].currentCount) {
          digitalWrite(servo[i].pin, LOW);
      }
  }
  
    // --- Szervó mozgás slice ---
    volatile servo_t& s = servo[currentServoIndex];
    if (s.enabled) {
        if (s.currentCount != s.targetCount) {
            if ((tickCounter - s.lastStepTick) >= s.stepIntervalTick) {
                if (s.currentCount < s.targetCount)
                    s.currentCount++;
                else
                    s.currentCount--;

                s.lastStepTick = tickCounter;

                // Félút jelzés
                if (!s.halfwayTriggered &&
                    ((s.startCount < s.targetCount && s.currentCount >= s.halfway) ||
                     (s.startCount > s.targetCount && s.currentCount <= s.halfway))) {
                    s.halfwayTriggered = true;
                    // Ide lehet plusz action pl. villogtatás
                }
            }
        }
        else if (!s.reachedCallbackCalled) {
            notifyServoReached(currentServoIndex);
            s.reachedCallbackCalled = true;
        }
    }

    // Következő szervóra lépés
    if (++currentServoIndex >= MAX_SERVOS) {
        currentServoIndex = 0;
    }

    // PWM ciklus számláló újraindítás
    if (timerCount++ >= REFRESH_INTERVAL / TIMER_INTERVAL_MICRO) {
        timerCount = 1;
    }
  
/* ez jó volt: 
  // --- PWM jel kezelés MINDEN szervóra ---
for (int i = 0; i < MAX_SERVOS; i++) {
  if (servo[i].enabled && (servo[i].pin <= STM32_WRONG_PIN)) {
      if (timerCount == servo[i].currentCount) {
          digitalWrite(servo[i].pin, LOW);
      }
      else if (timerCount == 1) {
          digitalWrite(servo[i].pin, HIGH);
      }
  }
}

// --- CSAK 1 szervót kezelünk egyszerre ---
if (servo[currentServoIndex].enabled && (servo[currentServoIndex].pin <= STM32_WRONG_PIN)) {
  if (servo[currentServoIndex].currentCount != servo[currentServoIndex].targetCount) {
      if ((timerMicros - servo[currentServoIndex].lastStepMicros) >= servo[currentServoIndex].stepIntervalMicros) {
          if (servo[currentServoIndex].currentCount < servo[currentServoIndex].targetCount)
              servo[currentServoIndex].currentCount++;
          else
              servo[currentServoIndex].currentCount--;

          servo[currentServoIndex].lastStepMicros = timerMicros;

          // --- Félút logika ---
          if (!servo[currentServoIndex].halfwayTriggered && servo[currentServoIndex].polPin != STM32_WRONG_PIN) {
              if ((servo[currentServoIndex].startCount < servo[currentServoIndex].targetCount && servo[currentServoIndex].currentCount >= servo[currentServoIndex].halfway) ||
                  (servo[currentServoIndex].startCount > servo[currentServoIndex].targetCount && servo[currentServoIndex].currentCount <= servo[currentServoIndex].halfway)) {
                  digitalWrite(servo[currentServoIndex].polPin, HIGH);
                  servo[currentServoIndex].halfwayTriggered = true;
              }
          }
      }
  }
  else { // célra ért
      if (!servo[currentServoIndex].reachedCallbackCalled) {
          if (notifyServoReached) notifyServoReached(currentServoIndex);
          //if(servo[servoIndex].autoOff) servo[servoIndex].enabled = false;
          servo[currentServoIndex].reachedCallbackCalled = true;
      }
  }
}

// --- Lépés a következő szervóra ---
if (++currentServoIndex >= MAX_SERVOS) {
  currentServoIndex = 0;
}

// --- PWM ciklus számláló ---
if (timerCount++ >= REFRESH_INTERVAL / TIMER_INTERVAL_MICRO) {
  timerCount = 1;
}

// --- Idő számláló ---
timerMicros += TIMER_INTERVAL_MICRO;

/*    for (int servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++) {
        if ( servo[servoIndex].enabled  && (servo[servoIndex].pin <= STM32_WRONG_PIN) ) {
            if ( timerCount == servo[servoIndex].currentCount ) {
                // PWM to LOW, will be HIGH again when timerCount = 1
                digitalWrite(servo[servoIndex].pin, LOW);
            }
            else if (timerCount == 1) {
                // PWM to HIGH, will be LOW again when timerCount = servo[servoIndex].count
                digitalWrite(servo[servoIndex].pin, HIGH);
            }
            
            if (servo[servoIndex].currentCount != servo[servoIndex].targetCount)
            {
                if ((timerMicros - servo[servoIndex].lastStepMicros) >= servo[servoIndex].stepIntervalMicros)
                {
                if (servo[servoIndex].currentCount < servo[servoIndex].targetCount)
                    servo[servoIndex].currentCount++;
                else
                    servo[servoIndex].currentCount--;

                servo[servoIndex].lastStepMicros = timerMicros;

                // --- Félút logika ---
                if (!servo[servoIndex].halfwayTriggered && servo[servoIndex].polPin!=STM32_WRONG_PIN)
                {
                  if ((servo[servoIndex].startCount < servo[servoIndex].targetCount && servo[servoIndex].currentCount >= servo[servoIndex].halfway) ||
                      (servo[servoIndex].startCount > servo[servoIndex].targetCount && servo[servoIndex].currentCount <= servo[servoIndex].halfway))
                  {
                    digitalWrite(servo[servoIndex].polPin, HIGH);  // vagy akár villogtatás, callback, stb.
                    servo[servoIndex].halfwayTriggered = true;
                  }
                }
              }
            }
            else { // célra ért
              if(!servo[servoIndex].reachedCallbackCalled) {
                if(notifyServoReached) notifyServoReached(servoIndex);
                //if(servo[servoIndex].autoOff) servo[servoIndex].enabled = false;
                servo[servoIndex].reachedCallbackCalled = true;
              }
            }
        }
    }

    // Reset when reaching 20000us / 10us = 2000
    if (timerCount++ >= REFRESH_INTERVAL / TIMER_INTERVAL_MICRO)
    {
        //ISR_SERVO_LOGDEBUG("Reset count");

        timerCount = 1;
    }

    timerMicros += TIMER_INTERVAL_MICRO;

    /*if (++updateDivider >= 100)  // csak minden 50. hívásra frissít
    {
        updateDivider = 0;
    }*/
  
}

// find the first available slot
// return -1 if none found
int STM32_ISR_Servo::findFirstFreeSlot()
{
  // all slots are used
  if (numServos >= MAX_SERVOS)
    return -1;

  // return the first slot with no count (i.e. free)
  for (uint8_t servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++)
  {
    if (servo[servoIndex].pin == STM32_WRONG_PIN)
    {
      ISR_SERVO_LOGDEBUG1("Index =", servoIndex);

      return servoIndex;
    }
  }

  // no free slots found
  return -1;
}

int8_t STM32_ISR_Servo::setupServo(uint8_t pin, bool autoOff, uint8_t speed, 
  uint8_t egyenesPos, uint8_t kiteroPos, uint8_t startPos,
  uint8_t polPin)
{
  int servoIndex;

  if (pin >= STM32_WRONG_PIN) {
    Serial.print("Wrong pin ");
    Serial.println(pin);
    return -1;
  }

  if (numServos < 0)
    init();

  servoIndex = findFirstFreeSlot();

  if (servoIndex < 0)
    return -1;

  servo[servoIndex].pin = pin;
  servo[servoIndex].polPin = polPin;
  servo[servoIndex].autoOff = autoOff;
  if(startPos>180) startPos=180;
  servo[servoIndex].targetCount  = map(startPos, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
  servo[servoIndex].currentCount = map(startPos, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
  servo[servoIndex].position = startPos;
  servo[servoIndex].reachedCallbackCalled = true;
  if(egyenesPos>180) egyenesPos=180;
  servo[servoIndex].egyenesAllasCount = map(egyenesPos, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
  if(kiteroPos>180) kiteroPos=180;
  servo[servoIndex].kiteroAllasCount = map(kiteroPos, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;

  setSpeed(servoIndex, speed);

  pinMode(pin, OUTPUT);

  if(polPin != STM32_WRONG_PIN) {
    pinMode(polPin, OUTPUT);
    digitalWrite(polPin, startPos==kiteroPos);
  }

  numServos++;

  ISR_SERVO_LOGDEBUG3("Index =", servoIndex, ", count =", servo[servoIndex].targetCount);
  //ISR_SERVO_LOGDEBUG3("min =", servo[servoIndex].min, ", max =", servo[servoIndex].max);

  // minden kész, indulhat a jel
  servo[servoIndex].enabled = true; 

  return servoIndex;
}

bool STM32_ISR_Servo::setPosition(uint8_t servoIndex, const float& position)
{
  if (servoIndex >= MAX_SERVOS)
    return false;

  // Updates interval of existing specified servo
  if ( servo[servoIndex].enabled && (servo[servoIndex].pin <= STM32_WRONG_PIN) )
  {    
    servo[servoIndex].startCount = servo[servoIndex].currentCount;

    servo[servoIndex].position = position;
    servo[servoIndex].targetCount = map(position, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;

    servo[servoIndex].halfway = (servo[servoIndex].startCount + servo[servoIndex].targetCount) / 2;
    servo[servoIndex].halfwayTriggered = false; // új mozgás indul, még nem volt félút
                                  
    ISR_SERVO_LOGDEBUG1("Idx =", servoIndex);
    ISR_SERVO_LOGDEBUG3("cnt =", servo[servoIndex].targetCount, ", pos =", servo[servoIndex].position);

    servo[servoIndex].reachedCallbackCalled = false;

    return true;
  }

  // false return for non-used numServo or bad pin
  return false;
}

bool STM32_ISR_Servo::setAllas(uint8_t servoIndex, bool kitero) {
  if(servo[servoIndex].pin <= STM32_WRONG_PIN) {
    servo[servoIndex].enabled = true;
    if(kitero) {
      return setPosition(servoIndex,servo[servoIndex].kiteroAllasCount);
    }
    else {
      return setPosition(servoIndex,servo[servoIndex].egyenesAllasCount);
    }
  }

  return false;
}

// returns last position in degrees if success, or -1 on wrong servoIndex
float STM32_ISR_Servo::getPosition(uint8_t servoIndex)
{
  if (servoIndex >= MAX_SERVOS)
    return -1.0f;

  // Updates interval of existing specified servo
  if ( servo[servoIndex].enabled && (servo[servoIndex].pin <= STM32_WRONG_PIN) )
  {
    ISR_SERVO_LOGERROR1("Idx =", servoIndex);
    ISR_SERVO_LOGERROR3("cnt =", servo[servoIndex].targetCount, ", pos =", servo[servoIndex].position);

    return (servo[servoIndex].position);
  }

  // return 0 for non-used numServo or bad pin
  return -1.0f;
}

bool STM32_ISR_Servo::setSpeed(uint8_t servoIndex, uint8_t speed) {
  if (servoIndex >= MAX_SERVOS)
    return false;

  servo[servoIndex].speed = speed;
  //servo[servoIndex].stepIntervalMicros = map(speed, 1, 255, MAX_STEP_INTERVAL, MIN_STEP_INTERVAL);
  servo[servoIndex].stepIntervalTick = map(speed, 1, 255, MAX_STEP_INTERVAL, MIN_STEP_INTERVAL);
  
  return true;
}

// setPulseWidth will set servo PWM Pulse Width in microseconds, correcponding to certain position in degrees
// by using PWM, turn HIGH 'pulseWidth' microseconds within REFRESH_INTERVAL (20000us)
// min and max for each individual servo are enforced
// returns true on success or -1 on wrong servoIndex
bool STM32_ISR_Servo::setPulseWidth(uint8_t servoIndex, uint16_t& pulseWidth)
{
  if (servoIndex >= MAX_SERVOS)
    return false;

  // Updates interval of existing specified servo
  if ( servo[servoIndex].enabled && (servo[servoIndex].pin <= STM32_WRONG_PIN) )
  {
    if (pulseWidth < MIN_PULSE_WIDTH) // servo[servoIndex].min)
      pulseWidth = MIN_PULSE_WIDTH; // servo[servoIndex].min;
    else if (pulseWidth > MAX_PULSE_WIDTH) // servo[servoIndex].max)
      pulseWidth = MAX_PULSE_WIDTH; // servo[servoIndex].max;

    servo[servoIndex].targetCount     = pulseWidth / TIMER_INTERVAL_MICRO;
    //servo[servoIndex].position  = map(pulseWidth, servo[servoIndex].min, servo[servoIndex].max, 0, 180);
    servo[servoIndex].position  = map(pulseWidth, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH, 0, 180);

    ISR_SERVO_LOGERROR1("Idx =", servoIndex);
    ISR_SERVO_LOGERROR3("cnt =", servo[servoIndex].targetCount, ", pos =", servo[servoIndex].position);

    return true;
  }

  // false return for non-used numServo or bad pin
  return false;
}

// returns pulseWidth in microsecs (within min/max range) if success, or 0 on wrong servoIndex
uint16_t STM32_ISR_Servo::getPulseWidth(uint8_t servoIndex)
{
  if (servoIndex >= MAX_SERVOS)
    return 0;

  // Updates interval of existing specified servo
  if ( servo[servoIndex].enabled && (servo[servoIndex].pin <= STM32_WRONG_PIN) )
  {
    ISR_SERVO_LOGERROR1("Idx =", servoIndex);
    ISR_SERVO_LOGERROR3("cnt =", servo[servoIndex].targetCount, ", pos =", servo[servoIndex].position);

    return (servo[servoIndex].targetCount * TIMER_INTERVAL_MICRO );
  }

  // return 0 for non-used numServo or bad pin
  return 0;
}


void STM32_ISR_Servo::deleteServo(uint8_t servoIndex)
{
  if ( (numServos == 0) || (servoIndex >= MAX_SERVOS) )
  {
    return;
  }

  // don't decrease the number of servos if the specified slot is already empty
  if (servo[servoIndex].enabled)
  {
    memset((void*) &servo[servoIndex], 0, sizeof (servo_t));

    servo[servoIndex].enabled   = false;
    servo[servoIndex].position  = 0;
    servo[servoIndex].targetCount     = 0;
    // Intentional bad pin, good only from 0-16 for Digital, A0=17
    servo[servoIndex].pin       = STM32_WRONG_PIN;

    // update number of servos
    numServos--;
  }
}

bool STM32_ISR_Servo::isEnabled(uint8_t servoIndex)
{
  if (servoIndex >= MAX_SERVOS)
    return false;

  if (servo[servoIndex].pin > STM32_WRONG_PIN)
  {
    // Disable if something wrong
    servo[servoIndex].pin     = STM32_WRONG_PIN;
    servo[servoIndex].enabled = false;
    return false;
  }

  return servo[servoIndex].enabled;
}

bool STM32_ISR_Servo::enable(uint8_t servoIndex)
{
  if (servoIndex >= MAX_SERVOS)
    return false;

  if (servo[servoIndex].pin > STM32_WRONG_PIN)
  {
    // Disable if something wrong
    servo[servoIndex].pin     = STM32_WRONG_PIN;
    servo[servoIndex].enabled = false;
    return false;
  }

  if ( servo[servoIndex].targetCount >= MIN_PULSE_WIDTH / TIMER_INTERVAL_MICRO )
    servo[servoIndex].enabled = true;

  return true;
}

bool STM32_ISR_Servo::disable(uint8_t servoIndex)
{
  if (servoIndex >= MAX_SERVOS)
    return false;

  if (servo[servoIndex].pin > STM32_WRONG_PIN)
    servo[servoIndex].pin     = STM32_WRONG_PIN;

  servo[servoIndex].enabled = false;

  return true;
}

void STM32_ISR_Servo::enableAll()
{
  // Enable all servos with a enabled and count != 0 (has PWM) and good pin

  for (uint8_t servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++)
  {
    if ( (servo[servoIndex].targetCount >= MIN_PULSE_WIDTH / TIMER_INTERVAL_MICRO ) && !servo[servoIndex].enabled
         && (servo[servoIndex].pin <= STM32_WRONG_PIN) )
    {
      servo[servoIndex].enabled = true;
    }
  }
}

void STM32_ISR_Servo::disableAll()
{
  // Disable all servos
  for (uint8_t servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++)
  {
    servo[servoIndex].enabled = false;
  }
}

bool STM32_ISR_Servo::toggle(uint8_t servoIndex)
{
  if (servoIndex >= MAX_SERVOS)
    return false;

  servo[servoIndex].enabled = !servo[servoIndex].enabled;

  return true;
}

int8_t STM32_ISR_Servo::getNumServos()
{
  return numServos;
}

#include "STM32_ServoDriver.h"

STM32_ServoDriver::STM32_ServoDriver()
    : numServos(-1)
{
}

static uint8_t currentServoIndex = 0;     // aktuális szervó index slice-hoz

void STM32_ServoDriver::run() {
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
                if (s.polPin!=STM32_WRONG_PIN && (s.config & SERVO_USE_POL) && !s.halfwayTriggered &&
                    ((s.startCount < s.targetCount && s.currentCount >= s.halfway) ||
                     (s.startCount > s.targetCount && s.currentCount <= s.halfway))) {
                
                    if (s.movingToDiverted) {
                        digitalWrite(s.polPin, HIGH); // Kitérő felé: HIGH
                    } else {
                        digitalWrite(s.polPin, LOW);  // Egyenes felé: LOW
                    }
                
                    s.halfwayTriggered = true;
                }
            }
        }
        else if (!s.reachedCallbackCalled) {
            notifyServoReached(currentServoIndex);
            s.reachedCallbackCalled = true;
            if(s.config & SERVO_AUTO_OFF) {
                s.enabled = false;
            }
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
}


// find the first available slot
// return -1 if none found
int STM32_ServoDriver::findFirstFreeSlot()
{
    // all slots are used
    if (numServos >= MAX_SERVOS)
        return -1;

    // return the first slot with no count (i.e. free)
    for (uint8_t servoIndex = 0; servoIndex < MAX_SERVOS; servoIndex++) {
        if (servo[servoIndex].pin == STM32_WRONG_PIN) {
            return servoIndex;
        }
    }

    // no free slots found
    return -1;
}


int8_t STM32_ServoDriver::setupServo(uint8_t pin, uint8_t config, uint8_t speed, 
    uint8_t egyenesPos, uint8_t kiteroPos, uint8_t startPos,
    uint8_t polPin) 
{
    int servoIndex;
  
    if (pin >= STM32_WRONG_PIN) {
        return -1;
    }
  
    if (numServos < 0) {
        init();
    }
  
    servoIndex = findFirstFreeSlot();
  
    if (servoIndex < 0) {
        return -1;
    }

    servo[servoIndex].pin = pin;
    servo[servoIndex].polPin = polPin;
    servo[servoIndex].config = config;
    if(startPos>180) startPos=180;
    servo[servoIndex].targetCount  = map(startPos, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
    servo[servoIndex].currentCount = map(startPos, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
    servo[servoIndex].targetPosition = startPos;
    servo[servoIndex].reachedCallbackCalled = true;
    if(egyenesPos>180) egyenesPos=180;
    servo[servoIndex].egyenesAllasPos = egyenesPos;
    if(kiteroPos>180) kiteroPos=180;
    servo[servoIndex].egyenesAllasPos = kiteroPos;

    calcHalfway(servoIndex);

    setSpeed(servoIndex, speed);
  
    pinMode(pin, OUTPUT);
    
    numServos++;
  
    // minden kész, indulhat a jel
    // kivéve - a start szekvencia indítja 
    // servo[servoIndex].enabled = true; 

    return servoIndex;
}

void STM32_ServoDriver::calcHalfway(uint8_t servoIndex) {
    uint16_t halfway = ((uint16_t)servo[servoIndex].egyenesAllasPos + servo[servoIndex].kiteroAllasPos) / 2;
    #ifdef SERVO_DEUG
        Serial.print("New halfway=");
        Serial.println(halfway);
    #endif
    servo[servoIndex].halfway = map(halfway, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
}

bool STM32_ServoDriver::setAllas(uint8_t servoIndex, bool kitero) {
    if(servoIndex >= MAX_SERVOS) {
        return false;
    }

    noInterrupts();
    servo[servoIndex].kiteroAllas = kitero;
    servo[servoIndex].startCount = servo[servoIndex].currentCount;
    servo[servoIndex].halfwayTriggered = false;
    servo[servoIndex].reachedCallbackCalled = false;
    servo[servoIndex].lastStepTick = tickCounter;
    
    if(kitero) {
        servo[servoIndex].targetPosition = servo[servoIndex].kiteroAllasPos;
        servo[servoIndex].movingToDiverted = true; 
        }
    else {
        servo[servoIndex].targetPosition = servo[servoIndex].egyenesAllasPos;
        servo[servoIndex].movingToDiverted = false; 
    }

    servo[servoIndex].targetCount = 
        map(servo[servoIndex].targetPosition, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH) / TIMER_INTERVAL_MICRO;
    servo[servoIndex].enabled = true;
    interrupts();

    return true;
}

bool STM32_ServoDriver::getAllas(uint8_t servoIndex) {
    if (servoIndex >= MAX_SERVOS)
        return false;

    return servo[servoIndex].kiteroAllas;
}

bool STM32_ServoDriver::setSpeed(uint8_t servoIndex, uint8_t speed) {
    if (servoIndex >= MAX_SERVOS)
        return false;
  
    #ifdef SERVO_DEUG
        Serial.print("Set speed to ");
        Serial.println(speed);
    #endif
    servo[servoIndex].speed = speed;
    servo[servoIndex].stepIntervalTick = map(speed, 1, 255, MIN_STEP_INTERVAL, MAX_STEP_INTERVAL);

    return true;
}

uint8_t STM32_ServoDriver::getSpeed(uint8_t servoIndex) {
    if (servoIndex >= MAX_SERVOS)
        return 0;

    return servo[servoIndex].speed;
}
  
bool STM32_ServoDriver::setEgyenesAllasPos(uint8_t servoIndex, uint8_t pos) {
    if (servoIndex >= MAX_SERVOS)
        return false;

    if(servo[servoIndex].egyenesAllasPos == pos)
        return true;

    bool needAllitas = servo[servoIndex].targetPosition == servo[servoIndex].egyenesAllasPos;
    if(pos>180) pos=180;
    servo[servoIndex].egyenesAllasPos = pos;
    calcHalfway(servoIndex);
    if(needAllitas) setAllas(servoIndex,false);
    return true;
}

uint8_t STM32_ServoDriver::getEgyenesAllasPos(uint8_t servoIndex) {
    if (servoIndex >= MAX_SERVOS)
        return 0;

    return servo[servoIndex].egyenesAllasPos;
}

bool STM32_ServoDriver::setKiteroAllasPos(uint8_t servoIndex, uint8_t pos) {
    if (servoIndex >= MAX_SERVOS)
        return false;

    if(servo[servoIndex].kiteroAllasPos == pos)
        return true;

    bool needAllitas = servo[servoIndex].targetPosition == servo[servoIndex].kiteroAllasPos;
    if(pos>180) pos=180;
    servo[servoIndex].kiteroAllasPos = pos;
    calcHalfway(servoIndex);
    if(needAllitas) setAllas(servoIndex,true);
    return true;
}

uint8_t STM32_ServoDriver::getKiteroAllasPos(uint8_t servoIndex) {
    if (servoIndex >= MAX_SERVOS)
        return 0;

    return servo[servoIndex].kiteroAllasPos;
}

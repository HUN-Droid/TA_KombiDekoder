#include <Arduino.h>

// ---- Konfigurációk ----
#define MAX_SERVOS 16
#define REFRESH_INTERVAL 20000UL        // Szervó PWM periódus: 20000us (20ms)
#define TIMER_INTERVAL_MICRO 10UL        // Timer megszakítás 10us-onként

// Definiáljuk az alap impulzus időket (ezek a szervótól függhetnek)
#define MIN_PULSE_WIDTH 1000   // 1000us = 0 fok
#define MAX_PULSE_WIDTH 2000   // 2000us = 180 fok
//#define MIN_PULSE_WIDTH         800       // the shortest pulse sent to a servo  
//#define MAX_PULSE_WIDTH         2450      // the longest pulse sent to a servo 

// ---- Struktúra a szervókhoz ----
struct Servo_t {
    uint8_t pin;
    uint16_t currentCount;
    uint16_t targetCount;
    uint16_t startCount;
    uint16_t halfway;
    bool enabled;
    bool halfwayTriggered;
    bool reachedCallbackCalled;
    uint32_t lastStepTick;
    uint32_t stepIntervalTick;
};

Servo_t servo[MAX_SERVOS];

// ---- Időzítők ----
volatile uint32_t tickCounter = 0;
volatile uint16_t timerCount = 1;
static uint8_t currentServoIndex = 0;

// ---- Timer példány ----
HardwareTimer *timer = nullptr;

// ---- Callback példa ----
void notifyServoReached(uint8_t servoIndex) {
    Serial.print("Servo ");
    Serial.print(servoIndex);
    Serial.println(" reached target.");
}

// ---- Hardveres timer ISR ----
void onTimer() {
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
    Servo_t& s = servo[currentServoIndex];
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
}

// ---- Szervó inicializálás ----
void setupServo(uint8_t index, uint8_t pin, float startUs, float targetUs, float speedMs) {
    if (index >= MAX_SERVOS) return;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    servo[index].pin = pin;
    servo[index].enabled = true;
    servo[index].startCount = (uint16_t)(startUs / TIMER_INTERVAL_MICRO);
    servo[index].currentCount = servo[index].startCount;
    servo[index].targetCount = (uint16_t)(targetUs / TIMER_INTERVAL_MICRO);
    servo[index].halfway = (servo[index].startCount + servo[index].targetCount) / 2;
    servo[index].halfwayTriggered = false;
    servo[index].reachedCallbackCalled = false;
    servo[index].lastStepTick = tickCounter;
    servo[index].stepIntervalTick = (uint32_t)((speedMs * 1000UL) / TIMER_INTERVAL_MICRO / abs((int)(servo[index].targetCount - servo[index].startCount)));

    Serial.print("Setup servo pin=");
    Serial.println(pin);
}

void setServoToDegree(uint8_t index, float degree, float speedMs = 1000) {
    if (index >= MAX_SERVOS) return;
    if (degree < 0.0f) degree = 0.0f;
    if (degree > 180.0f) degree = 180.0f;

    // Átszámítás fokból PWM időtartamra
    float pulseWidth = MIN_PULSE_WIDTH + (degree / 180.0f) * (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH);

    // Átszámítás timerCount egységre
    uint16_t targetCount = (uint16_t)(pulseWidth / TIMER_INTERVAL_MICRO);

    // Beállítás
    noInterrupts();
    servo[index].startCount = servo[index].currentCount;
    servo[index].targetCount = targetCount;
    servo[index].halfway = (servo[index].startCount + servo[index].targetCount) / 2;
    servo[index].halfwayTriggered = false;
    servo[index].reachedCallbackCalled = false;
    servo[index].lastStepTick = tickCounter;
    servo[index].stepIntervalTick = (uint32_t)((speedMs * 1000UL) / TIMER_INTERVAL_MICRO / abs((int)(servo[index].targetCount - servo[index].startCount)));
    interrupts();
}

// ---- Setup ----
void setupIsrServo() {
    Serial.println("Starting STM32 PWM Synchronized ISR Servo Controller...");


    // Timer indítása
    timer = new HardwareTimer(TIM2);
    timer->pause();
    timer->setOverflow(1000000UL / (1000000UL / TIMER_INTERVAL_MICRO), HERTZ_FORMAT); // 100kHz timer -> 10us period
    timer->attachInterrupt(onTimer);
    timer->resume();
}


#ifndef MOTOSTM32F1_H
#define MOTOSTM32F1_H
// STM32F1 specific defines for Cpp files

//#warning STM32F1 specific cpp includes
extern uint8_t noStepISR_Cnt;   // Counter for nested StepISr-disable

void seizeTimerAS();
static inline __attribute__((__always_inline__)) void _noStepIRQ() {
    //timer_disable_irq(MT_TIMER, TIMER_STEPCH_IRQ);
    *bb_perip(&(MT_TIMER->regs).adv->DIER, TIMER_STEPCH_IRQ) = 0;
    noStepISR_Cnt++;
    //noInterrupts();
    //nvic_globalirq_disable();
    #if defined COMPILING_MOTOSTEPPER_CPP
        //Serial.println(noStepISR_Cnt);
        SET_TP3;
    #endif
}
static inline __attribute__((__always_inline__)) void  _stepIRQ(bool force = false) {
    //timer_enable_irq(MT_TIMER, TIMER_STEPCH_IRQ) cannot be used, because this also clears pending irq's
    if ( force ) noStepISR_Cnt = 1;              //enable IRQ immediately
    if ( noStepISR_Cnt > 0 ) noStepISR_Cnt -= 1; // don't decrease if already 0 ( if enabling IRQ is called too often )
    if ( noStepISR_Cnt == 0 ) {
        #if defined COMPILING_MOTOSTEPPER_CPP
            CLR_TP3;
        #endif
        *bb_perip(&(MT_TIMER->regs).adv->DIER, TIMER_STEPCH_IRQ) = 1;
        //nvic_globalirq_enable();
        //interrupts();
    }
    //Serial.println(noStepISR_Cnt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
#if defined COMPILING_MOTOSERVO_CPP
// Values for Servo: -------------------------------------------------------
constexpr uint8_t INC_PER_MICROSECOND = 8;		// one speed increment is 0.125 µs
constexpr uint8_t  COMPAT_FACT = 1; 			// no compatibility mode for stm32F1                       
constexpr uint8_t INC_PER_TIC = INC_PER_MICROSECOND / TICS_PER_MICROSECOND;
#define time2tic(pulse)  ( (pulse) *  INC_PER_MICROSECOND )
#define tic2time(tics)  ( (tics) / INC_PER_MICROSECOND )
#define AS_Speed2Inc(speed) (speed)
//-----------------------------------------------------------------

void ISR_Servo( void );


static inline __attribute__((__always_inline__)) void enableServoIsrAS() {
    timer_attach_interrupt(MT_TIMER, TIMER_SERVOCH_IRQ, ISR_Servo );
    timer_cc_enable(MT_TIMER, SERVO_CHN);
}

static inline __attribute__((__always_inline__)) void setServoCmpAS(uint16_t cmpValue) {
	// Set compare-Register for next servo IRQ
	timer_set_compare(MT_TIMER,  SERVO_CHN, cmpValue);
}	

#endif // COMPILING_MOTOSERVO_CPP



#endif
#ifndef MOTODEBUG_H
#define MOTODEBUG_H
// die folgenden defines werden im aufrufenden cpp-File gesetzt.
// so können die debugs klassenspezifisch eingeschaltet werden
//#define debugTP
//#define debugPrint

// über diese undefs kann das Debugging global abgeschaltet werden
#undef debugTP
#undef debugPrint

#ifdef debugTP 
//    #elif defined (__STM32F1__)
        //Test-HW füer Stepper:
        #define TP1 PB1
        #define TP2 PB0
        #define TP3 PB13 
        #define TP4 PB15
        #define MODE_TP1 pinMode( TP1,OUTPUT )   // TP1= PA1
        #define SET_TP1  digitalWrite( TP1, HIGH )
        #define CLR_TP1  digitalWrite( TP1, LOW )
        #define MODE_TP2 pinMode( TP2,OUTPUT )  
        #define SET_TP2  digitalWrite( TP2, HIGH )
        #define CLR_TP2  digitalWrite( TP2, LOW )
        #define MODE_TP3 pinMode( TP3,OUTPUT )   // 
        #define SET_TP3  digitalWrite( TP3, HIGH )
        #define CLR_TP3  digitalWrite( TP3, LOW )
        #define MODE_TP4 pinMode( TP4,OUTPUT )   // 
        #define SET_TP4  digitalWrite( TP4, HIGH )
        #define CLR_TP4  digitalWrite( TP4, LOW )

    
#else
    #define MODE_TP1 
    #define SET_TP1 
    #define CLR_TP1 
    #define MODE_TP2 
    #define SET_TP2 
    #define CLR_TP2 
    #define MODE_TP3 
    #define SET_TP3 
    #define CLR_TP3
    #define MODE_TP4 
    #define SET_TP4 
    #define CLR_TP4 
// special Testpoint for Servo-testing
    #define SET_SV3 
    #define CLR_SV3 
    #define SET_SV4 
    #define CLR_SV4 

#endif
        //#define MODE_TP3 pinMode( PB14,OUTPUT )   // TP3 = PB14
        //#define SET_SV3  gpio_write_bit( GPIOB,14, HIGH )
        //#define CLR_SV3  gpio_write_bit( GPIOB,14, LOW )
        //#define MODE_TP4 pinMode( PB15,OUTPUT )   // TP4 = PB15
        //#define TOG_TP4  gpio_toggle_bit( GPIOB,15)
        //#define TOG_TP2  gpio_toggle_bit( GPIOB,13)
        #define TOG_TP4
        #define TOG_TP2


#ifdef debugPrint
    #warning "Debug-printing is active"
	#ifdef ARDUINO_ARCH_AVR
        //#define DB_PRINT( x, ... ) {char dbgBuf[80]; sprintf_P( dbgBuf, PSTR( x ), ##__VA_ARGS__ ) ; Serial.println( dbgBuf ); }
        #define DB_PRINT( x, ... ) {char dbgBuf[80]; snprintf_P( dbgBuf, 80, PSTR( x ), ##__VA_ARGS__ ) ; Serial.println( dbgBuf ); }
        //#define DB_PRINT( x, ... ) {char dbgBuf[80]; sprintf( dbgBuf,  x , ##__VA_ARGS__ ) ; Serial.println( dbgBuf ); }
    #elif defined ARDUINO_ARCH_ESP32
        #define DB_PRINT( x, ... ) Serial.printf(  x , ##__VA_ARGS__ ) 
    #else
        #define DB_PRINT( x, ... ) {char dbgBuf[80]; sprintf( dbgBuf,  x , ##__VA_ARGS__ ) ; Serial.println( dbgBuf ); }
    #endif
    extern const char *rsC[] ;    
#else
    #define DB_PRINT( x, ... ) ;
#endif

#endif
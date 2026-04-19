#ifndef MOBATOOLS_H
#define MOBATOOLS_H
/*
  MobaTools.h - a library for model makers - and others too ;-) 
  Author: Franz-Peter Müller, f-pm+gh@mailbox.org
  Copyright (c) 2023 All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  MobaTools V2.6.1
   
  History:
  V2.6.2 9-2024
    - ESP32 core version 3.x is supported ( V2.x is still supported too )
	- fixed endless rotating when setting moveTo very quickly. (issue#34 on github) 
  V2.6.1 12-2023
    - bugfix with UNO R4Wifi and steppers (with Wifi active)
    - 2 more examples ( button matrix and UNO R4 Wifi and stepper )
  V2.6.0 12-2023
    - Support UNO R4 (Minima and WiFi)
	- MoToStepper.read allows reading the angle in fractions
	- internal optimizations
  V2.5.1 10-2023
    - Fix bug when setting stepper speed to 0 multiple times. The stepper stopped immediately when setting
      speed to 0 again while the stepper was ramping down from the first setting to 0.
	- Fix some bugs when setting low delay times on stepper enable.
  V2.5.0 09-2023
	- ESP32 board manager V2.x is supported, but the new HW variants (S2,S3,C3) are not yet supported
	- ATmega4809 is supported ( Nano Every, UNO WiFi Rev2 )
	- .setSpeedSteps(0) is allowed now and stops the stepper without loosing the target position
	- .getSpeedSteps() indicates direction of movement ( negative values means moving backwards )
	- .attachEnable( int delayTime ) allows disabling of 4-pin steppers (FULLSTEP/HALFSTEP) without
	  an extra enable pin ( all outputs are set to 0 ). This also works when connected via SPI.
	- some more examples
  V2.4.3 04-2022
	 - bugfix for setZero(position) for steppers in FULLSTEP mode
	 - bugfix with AccelStepper like method names ( compiler error if both libs have been included )
	 - 2 additional timer examples commented in english
	 - 1 additional stepper example
  V2.4.2 12-2021
	 - fix bug in MoToStepper.setSpeedSteps ( was possible divide by zero ), ESP crashed
  V2.4.1 11-2021
     - fix typo ( arduino.h -> Arduino.h ). This created an error on linux.
     - some fixes in MoToButtons and documentation
  V2.4.0 05-2021
     - ESP32 prozessors are supported
     - some ATtiny are supported ( needs a 16bit-timer1 amd a SPI or USI hardware )
     - Step-tuning for 32Bit prozessors ( except ESP8266 ) for higher steprates
     - more examples
  V2.3.1 11-2020
     - Fix error with doSteps(0) and no ramp. Motor did not stop
  V2.3 07-2020
     - New class MoToTimebase to create events in regular intervals
     - MoToButton: The longpress event is already triggered when the time for longpress expires, not when the button is released
     - MoToStepper: steps per rotation can be changed together with setting a new reference point.
     
  V2.2 03-2020
  V2.1 02-2020
      - new class 'MoToButtons' to manage up to 32 buttons/switches
        ( debounce, events 'pressed', 'released', 'short press' and 'long press'
      - MoToTimer: new method 'expired', which is only 'true' at the first call 
        after timer expiry.
  V2.0 01-2020
      - classnames changed ( the old names can still be used for compatibility, 
        but should not be used in new sketches)
      - ESP8266 is supported
      - it is possible to define an enable pin for steppers. This is active
        only when the stepper is moving.
      - new method 'getSpeedSteps' returns actual speed
  V1.1.5 12-2019
      - Servo: fix error when 1st and 2nd write ( after attach ) are too close together
  V1.1.4 09-2019
      - speed = 0 is not allowsed( it is set to 1 internally )
	  - fix error when repeatedly setting target position very fast
	  - allow higher steprates up to 2500 steps/sec.
	    ( relative jitter increases with higher rates, abs. jitter is 200µs )
	  - typo corrected in MoToBase.h 
  V1.1.3 08-2019
      - no more warnings when compiling
      - fix error (overflow) when converting angle to steps
      - fix error when converting angle to microseconds in servo class
      - reworked softleds. Risetimes unitl 10 sec are possible.
  V1.1.2 08-2019
      - fix error when only servo objects are defined ( sketch crashed )
      - two more stepper examples ( thanks to 'agmue' from german arduino.cc forum )
      - detach() configures used pins back to INPUT
  V1.1 07-2019
      - stepper now supports ramps (accelerating, decelerating )
      - stepper speed has better resolution with high steprates
      - split source into several fuction-specific .cpp files. This
        saves flash space if only part of the functionality is used.
  V1.0  11-2017 Use of Timer 3 if available ( on AtMega32u4 and AtMega2560 )
  V0.9  03-2017
        Better resolution for the 'speed' servo-paramter (programm starts in compatibility mode)
        outputs for softleds can be inverted
        MobaTools run on STM32F1 platform
        
  V0.8 02-2017
        Enable Softleds an all digital outputs
  V0.7 01-2017
		Allow nested Interrupts with the servos. This allows more precise other
        interrupts e.g. for NmraDCC Library.
		A4988 stepper driver IC is supported (needs only 2 ports: step and direction)

   
*/



// defines that may be changed by the user

    
// default CYCLETIME is processordependent, change only if you know what you are doing ).
#define MIN_STEP_CYCLE  25   // Minimum number of µsec  per step 


// servo related defines
#define MINPULSEWIDTH   700U      // don't make it shorter than 700
#define MAXPULSEWIDTH   2300U     // don't make it longer than 2300


//  !!!!!!!!!!!!  Don't change anything after tis line !!!!!!!!!!!!!!!!!!!!
 

#include <MoToBase.h>
#include <MoToServo.h>
//#include <utilities/MoToPwm.h>

//#include <MoToSTM32F1.h>
#include "driverSTM32.h"

#ifndef INTERNALUSE
//#include <MoToButtons.h>
//#include <MoToTimer.h>
#endif

#ifdef debug
#include <utilities/MoToDbg.h>
#endif

#endif


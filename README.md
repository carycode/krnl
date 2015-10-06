# KRNL - a small preemptive kernel for small systems 
       
I have found it interesting to develop an open source realtime kernel for the Arduino platform - but is also portable to other platforms

* latest version 2001 *


changes about clip of semaphores so Liy & Layland RMA is detectable (when it fails deadlines)

- SEE SOME NOTES BELOW ABOUT TIMERS AND PINS 
- Now doxygen docu at html directory :-)
- See krnl.h for further comments
- - timers
- - 8/16 MHz setting
- - etc

See github.com/jdn-aau  or http://www.control.auc.dk/~jdn/edu/doc/arduino/krnl/

See some warnings in the bottom !!!

## Some highlights


- open source (beer license)
- well suited for teaching
 - easy to read and understand source
 - priority to straight code instead insane optimization(which will make it nearly unreadable)

- well suited for serious duties - but with no warranty what so ever - only use it at own risk !!!

- easy to use
 - just import library krnl and you are ready

- automatic recognition of Arduino architeture
 - supports all atmega variants I have had available (168,328,1280,2560 - uno, duemillanove, mega 1280 and 2560)
- Priority ceiling supported with k_prio_wait and k_prio_signal where you supply with task priorities

## Some characteristics:


- Mmeory usage
As of vrs 2001 
-- static usage around 70B
-- a semaphore/task occpy 18B in krnl data structure
-- a msg-Q occupy 17B plus 18B(an internal semaphore)
-- the two above allocated from heap

So k_init(2,3,4) gives 71B global (static) and 18*((2+1)+(3+1)+4) + 4*17 = 266B which 

the +1 on task for a descriptor for main/dummy
the +1 on semaphore for the timer/sleep semaphore

And then you have to add up arrays for stack for each task. Less than 50B is not a good idea.
You can runtime check how deep the stack has been used by k_unused_stak


- preemptive scheduling 
 - Basic heart beat at 1 kHz. SNOT can have heeartbeat in quants of milli seconds
 - static preemptivepriority scheme
-  support task, semaphores, message queues
 - All elements shall be allocated prior to start of KRNL
- support user ISRs and external interrupts

## Timers

The Arduino has 3 or 6 timers (Mega has 6 the rest has 3)

### timer0
- Timer0 and 2  is a 8bit timer.
- In the Arduino world Timer0 is been used for the timer functions, like delay(), millis() and micros().
-  If you change Timer0 registers, this may influence the Arduino timer function.
- So you should know what you are doing.

### timer1
- Timer1 is a 16bit timer.
- In the Arduino world the Servo library uses Timer1 on Arduino Uno (Timer5 on Arduino Mega).

### timer2
- Timer2 is a 8bit timer like Timer0.
- In the Arduino work the tone() function uses Timer2.

### timer3, timer4, timer5
- timer 3,4,5 are only available on Arduino Mega boards.
- These timers are all 16bit timers.

- krnl can be configures to use tmr 1,2 and for mega also 3,4,5 for running krnl tick
- For timer 0 you should take care of millis and it will require some modifications in arduino lib
- see krnl.h for implications

Accuracy

- 8 bit timers (0,2) 1 millisecond is 15.625 countdown on timer
   - example 10 msec 156 instead of 156.25 so an error of 0.25/156.25 ~= 0.2%
 - 16 bit timers count down is 1 millisecond for 62.5 count
 - - example 10 msec ~ 625 countdown == precise :-)

 - timers default
  - all except MEGA use timer 1 ( 8 bit)
  - MEGAs (1280/2560) use timer 5
  - you can change it in krnl.h 

### timer quants  (heartbeat)
k_start accepts 1..10 and 20,30,40,...10000 milliseconds timer quants
So you can not run krnl with an internal timer at 16 msec
You can change it in k_start but be aware of implications of 8/16 bit timer usage


### Overflow detection
The user can provide two functions which will be called when a semaphore/msgQ are overflooding

### warning / info
... from http://arduino-info.wikispaces.com/Timers-Arduino

- Servo Library uses Timer1. 
- -  You can’t use PWM on Pin 9, 10 when you use the Servo Library on an Arduino. 
- -  For Arduino Mega it is a bit more difficult. The timer needed depends on the number of servos. 
- -  Each timer can handle 12 servos. 
- -  For the first 12 servos timer 5 will be used (losing PWM on Pin 44,45,46). 
- -  For 24 Servos timer 5 and 1 will be used (losing PWM on Pin 11,12,44,45,46).. 
- -  For 36 servos timer 5, 1 and 3 will be used (losing PWM on Pin 2,3,5,11,12,44,45,46).. 
- -  For 48 servos all 16bit timers 5,1,3 and 4 will be used (losing all PWM pins).

- Pin 11 has shared functionality PWM and MOSI. 
- -  MOSI is needed for the SPI interface, You can’t use PWM on Pin 11 and the SPI interface at the same time on Arduino. 
- -  On the Arduino Mega the SPI pins are on different pins.

- tone() function uses at least timer2. 
- -  You can’t use PWM on Pin 3,11 when you use the tone() function an Arduino and Pin 9,10 on Arduino Mega.

-----

See in krnl.h for information like ...

... from http://blog.oscarliang.net/arduino-timer-and-interrupt-tutorial/

-----


Install from github:

1) cd whatever/sketchbook/libraries   - see Preferences for path to sketchbook
2) git clone https://github.com/jdn-aau/krnl.git


## Warning 
You have from Arduino inherited many critical regions which you have to protect - like

- Serial channels - only on thread at time must have access
- digital and analog IO (digitalRead, AnalogRead,...)
- and in general all libraries - so take care

This is NOT an Ardunio problem but a standard feature i multithreaded systems :-)

(c)
* "THE BEER-WARE LICENSE" (frit efter PHK)           *
 * <jdn@es.aau.dk> wrote this file. As long as you    *
 * retain this notice you can do whatever you want    *
 * with this stuff. If we meet some day, and you think*
 * this stuff is worth it ...                         *
 *  you can buy me a beer in return :-)               *
 * or if you are real happy then ...                  *
 * single malt will be well received :-)              *
 *                                                    *
 * Use it at your own risk - no warranty       



Happy hacking

See also http://www.control.auc.dk/~jdn/edu/doc/arduino/krnl/

/Jens


and https://github.com/greiman/FreeRTOS-Arduino
has freertos for Arduino and Arduino Due

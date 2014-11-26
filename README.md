      >>>  KRNL - a small preemptive kernel for small systems <<<
       
I have found it interesting to develop an open source realtime kernel 

for the Arduino platform - but is also portable to other platforms



- SEE SOME NOTES BELOW ABOUT TIMERS AND PINS 
- Now doxygen docu at https://github.com/jdn-aau/krnl/html/index.html

Some highlights
---------------

- open source (beer license)
- well suited for teaching
-- easy to read and understand source
-- priority to straight code instead insane optimization(which will make it nearly unreadable)

- well suited for serious duties - but with no warranty what so ever - only use it at own risk !!!

- easy to use
-- just import library krnl and you are ready

- automatic recognition of Arduino architeture
-- supports all atmega variants I have had available (168,328,1280,2560 - uno, duemillanove, mega 1280 and 2560)
Some characteristics:

- preemptive scheduling 
-- Basic heart beat at 1 kHz. SNOT can have heeartbeat in quants of milli seconds
-- KRNL uses timer2 so PWM based on timer2 is not possible(you can change it)
-- static priority scheme
- support task, semaphores, message queues
-- All elements shall be allocated prior to start of KRNL
- support user ISRs and external interrupts

Install from github:

1) cd whatever/sketchbook/libraries   - see Preferences for path to sketchbook
2) git clone https://github.com/jdn-aau/krnl.git

NB NB NB - TIMER HEARTBEAT
 From vrs 1236 you can change which timer to use in krnl.c Just look in top of file for KRNLTMR
 - tested with uno and mega 256

In krnl.c you can configure KRNL to use timer 0,1,2,3,4 or 5.

You can select heartbeat between 1 and 200 milliseconds in 1 msec steps.


- Timer0 - An 8 bit timer used by Arduino functions delay(), millis() and micros().
- Timer1 - A 16 bit timer used by the Servo() library
- Timer2 - An 8 bit timer used by the Tone() library
- Timer3,4,5 16 bits
    
    
... from http://arduino-info.wikispaces.com/Timers-Arduino

- Servo Library uses Timer1. 
--  You can’t use PWM on Pin 9, 10 when you use the Servo Library on an Arduino. 
--  For Arduino Mega it is a bit more difficult. The timer needed depends on the number of servos. 
--  Each timer can handle 12 servos. 
--  For the first 12 servos timer 5 will be used (losing PWM on Pin 44,45,46). 
--  For 24 Servos timer 5 and 1 will be used (losing PWM on Pin 11,12,44,45,46).. 
--  For 36 servos timer 5, 1 and 3 will be used (losing PWM on Pin 2,3,5,11,12,44,45,46).. 
--  For 48 servos all 16bit timers 5,1,3 and 4 will be used (losing all PWM pins).

- Pin 11 has shared functionality PWM and MOSI. 
--  MOSI is needed for the SPI interface, You can’t use PWM on Pin 11 and the SPI interface at the same time on Arduino. 
--  On the Arduino Mega the SPI pins are on different pins.

- tone() function uses at least timer2. 
--  You can’t use PWM on Pin 3,11 when you use the tone() function an Arduino and Pin 9,10 on Arduino Mega.


Happy hacking

See also http://es.aau.dk/staff/jdn/edu/doc/arduino/krnl

/Jens

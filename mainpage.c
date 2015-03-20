/*! \mainpage KRNL - a Simple portable kernel for Ardunio 
 * this file only for doxygen use 
 *
 *
 * my own small KeRNeL adapted for Arduino - now known as KRNL
* (old name was SNOT)
 *
 * (C) 2012,2013,2014,2015
 *
 * Jens Dalsgaard Nielsen <jdn@es.aau.dk>
 * http://www.control.aau.dk/~jdn
 * Section of Automation & Control
 * Aalborg University,
 * Denmark
 *
 * "THE BEER-WARE LICENSE" (frit efter PHK)
 * <jdn@es.aau.dk> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return :-) or if you are real happy then ...
 * single malt will be well received :-)
 *
 * (C) Jens Dalsgaard Nielsen
 *
 * \section Introduction
 * KRNL is a preemptive realtime kernel here ported for Arduino
 *
 * See krnl.h for documentation or follow the links below
*
* NEWS : get it from github Q see https://github.com/jdn-aau/krnl
* \tableofcontents

 One note before starting:


\section  a1 Before you start

All initialization must be carried out after k_init and BEFORE k_start

So no creation of semaphores, tasks, etc when the system is running.

Why ? because we are dealing with embedded systems and once your system is configured it shall just run.

I do also really advise not to use dynamic memory after k_start: no malloc and free when your system is running.

If ... you need to do so please protect memory by a mutex (semaphore + k_wait and k_signal) or just encapsule malloc in DI() and EI().
And never never use and rely on free. You may end up with fragmented memory which on the long run is of no use.

So the advise is to

- create all tasks, semaphores etc
- allocate what is needed of memory, resources etc
- and then k_start

\section a112 Memory usage

Use freeRam for testing (part of krnl)

- Task descriptor 17B
- Semaphore descriptor 17B
- Message Queue descriptor 33B (17B for semaphore + 16B)


On the 1280/2560 a few more bytes is used due to extended program counter register

KRNL itself uses approx 190B before allocating task/sem/msgQ descriptors

\section a3 Initialization
If any call fails (like no more RAM or bad parameters) KRNL will not start but will return an error code in k_start
- int k_init(int nrTask, int nrSem, int nrMsg);
- int k_start(int tm); // tm in milliseconds
- int k_stop(int exitVal); // exitVal that will be returned by k_start to main starter

\section a4 Creation calls - before k_start
\subsection a41 Semaphore
- struct k_t * k_crt_sem(char init_val, int maxvalue);
\subsection a42 Task
- struct k_t * k_crt_task(void (*pTask)(void), char prio, char *pStk, int stkSize);
\subsection a43 Message Queue
- struct k_msg_t * k_crt_send_Q(int nr_el, int el_size, void *pBuf);


\section a55 Semaphore runtime calls
\subsection a5 User space
- char k_set_sem_timer(struct k_t * sem, int val);
- char k_signal(struct k_t * sem);
- char k_wait(struct k_t * sem, int timeout);

\subsection a51 ISR space
_i indicates that no lock/unlock(disable/enable) is carried out

- char ki_signal(struct k_t * sem);
- char ki_nowait(struct k_t * sem);
- char ki_wait(struct k_t * sem, int timeout);
- int ki_semval(struct k_t * sem);

\section a61 Message Queue calls
\subsection a611 User space
- char k_send(struct k_msg_t *pB, void *el);
- char k_receive(struct k_msg_t *pB, void *el, int timeout, int *lost_msg);

\subsection a612 ISR space
- char ki_send(struct k_msg_t *pB, void *el);
- char ki_receive(struct k_msg_t *pB, void *el, int * lost_msg);

\section a8 Task calls
- void ki_task_shift(void) __attribute__ ((naked));
- char k_sleep(int time);
- char k_set_prio(char prio);
- int k_stk_chk(struct k_t *t);

\section a9 Div calls
- int k_unused_stak(struct k_t *t);
- int freeRam(void);




      >>>  KRNL - a small preemptive kernel for small systems <<<
       
I have found it interesting to develop an open source realtime kernel 

for the Arduino platform - but is also portable to other platforms



- SEE SOME NOTES BELOW ABOUT TIMERS AND PINS 
- Now doxygen docu at html directory :-)
- See krnl.h for further comments
- - timers
- - 8/16 MHz setting
- - etc

See some warnings in the bottom !!!

Some highlights
---------------

- open source (beer license)
- well suited for teaching
 - easy to read and understand source
 - priority to straight code instead insane optimization(which will make it nearly unreadable)

- well suited for serious duties - but with no warranty what so ever - only use it at own risk !!!

- easy to use
 - just import library krnl and you are ready

- automatic recognition of Arduino architeture
 - supports all atmega variants I have had available (168,328,1280,2560 - uno, duemillanove, mega 1280 and 2560)
Some characteristics:

- preemptive scheduling 
 - Basic heart beat at 1 kHz. SNOT can have heeartbeat in quants of milli seconds
 - static priority scheme
-  support task, semaphores, message queues
 - All elements shall be allocated prior to start of KRNL
- support user ISRs and external interrupts

- timers
 - krnl can be configures to use tmr 1,2 and for mega also 3,4,5 for running krnl tick
 - For timer 0 you should take care of millis and it will require some modifications in arduino lib
 - see krnl.h for implications (like 

- Accuracy
 - 8 bit timers (0,2) 1 millisecond is 15.625 countdown on timer
   - example 10 msec 156 instead of 156.25 so an error of 0.25/156.25 ~= 0.2%
 - 16 bit timers count down is 1 millisecond for 62.5 count
 - - example 10 msec ~ 625 countdown == precise :-)

See in krnl.h for information like ...

... from http://blog.oscarliang.net/arduino-timer-and-interrupt-tutorial/
Timer0:
- Timer0 and 2  is a 8bit timer.
- In the Arduino world Timer0 is been used for the timer functions, like delay(), millis() and micros().
-  If you change Timer0 registers, this may influence the Arduino timer function.
- So you should know what you are doing.

Timer1:
- Timer1 is a 16bit timer.
- In the Arduino world the Servo library uses Timer1 on Arduino Uno (Timer5 on Arduino Mega).

Timer2:
- Timer2 is a 8bit timer like Timer0.
- In the Arduino work the tone() function uses Timer2.

Timer3, Timer4, Timer5: Timer 3,4,5 are only available on Arduino Mega boards.
- These timers are all 16bit timers.


Install from github:

1) cd whatever/sketchbook/libraries   - see Preferences for path to sketchbook
2) git clone https://github.com/jdn-aau/krnl.git

NB NB NB - TIMER HEARTBEAT
 From vrs 1236 you can change which timer to use in krnl.c Just look in top of file for KRNLTMR
 - tested with uno and mega 256

In krnl.c you can configure KRNL to use timer (0),1,2,3,4 or 5. (3,4,5 only for 1280/2560 mega variants)

You can select heartbeat between 1 and 200 milliseconds in 1 msec steps.

- Timer0 - An 8 bit timer used by Arduino functions delay(), millis() and micros(). BEWARE
- Timer1 - A 16 bit timer used by the Servo() library
- Timer2 - An 8 bit timer used by the Tone() library
- Timer3,4,5 16 bits
    
    
... from http://arduino-info.wikispaces.com/Timers-Arduino

- Servo Library uses Timer1. 
- -  You can’t use PWM on Pin 9, 10 when you use the Servo Library on an Arduino. 
- -  For Arduino Mega it is a bit more difficult. The timer needed depends on the number of servos. 
- -  Each timer can handle 12 servos. 
- -  For the first 12 servos timer 5 will be used (losing PWM on Pin 44,45,46). -
 -  For 24 Servos timer 5 and 1 will be used (losing PWM on Pin 11,12,44,45,46).. 
- -  For 36 servos timer 5, 1 and 3 will be used (losing PWM on Pin 2,3,5,11,12,44,45,46).. 
- -  For 48 servos all 16bit timers 5,1,3 and 4 will be used (losing all PWM pins).

- Pin 11 has shared functionality PWM and MOSI. 
- -  MOSI is needed for the SPI interface, You can’t use PWM on Pin 11 and the SPI interface at the same time on Arduino. 
- -  On the Arduino Mega the SPI pins are on different pins.

- tone() function uses at least timer2. 
- -  You can’t use PWM on Pin 3,11 when you use the tone() function an Arduino and Pin 9,10 on Arduino Mega.


Warning 1)

There has been found some problems with int/flot conversion to ascii strings in conjugation with printing and user written interrupt service routines.

It is time of writing (March 2015) unclear what the problem is but you may experience a frozen system - but it is burried some where in the C++ supplied library.

The solution is to do own numer to string conversion and the use Serial.print to print the string

   int i;
   Serial.print(i);

See some example code at  https://github.com/jdn-aau/i2a for int and long for conversion

Warning 2)
You have from Arduino inherited many critical regions which you have to protect - like

- Serial channels - only on thread at time must have access
- digital and analog IO (digitalRead, AnalogRead,...)
- and in general all libraries - so take care

This is NOT an Ardunio problem but standard i multithreaded systems

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

See also http://es.aau.dk/staff/jdn/edu/doc/arduino/krnl - but git is most updated

/Jens

*/
      >>>  KRNL - a small preemptive kernel for small systems <<<
       
I have found it interesting to develop an open source realtime kernel 

for the Arduino platform - but is also portable to other platforms

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

NB NB NB 
    Timer0 - An 8 bit timer used by Arduino functions delay(), millis() and micros().
    Timer1 - A 16 bit timer used by the Servo() library
    Timer2 - An 8 bit timer used by the Tone() library
    Timer3,4,5 16 bits
    
 From vrs 1236 you can change which timer to use in krnl.c Just look in top of file for KRNLTMR
 - tested with uno and mega 256
 

Happy hacking

See also http://es.aau.dk/staff/jdn/edu/doc/arduino/krnl

/Jens

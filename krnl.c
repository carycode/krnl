/*****************************************************
*          krnl.c  part of kernel KRNL               *
*         based on "snot"                            *
*                                                    *
*  June,               2014                          *
*  Feb                 2015                          *
*      Author: jdn                                   *
*                                                    *
******************************************************
*                                                    *
*            (simple not - not ?! :-) )              *
* my own small KeRNeL adapted for Arduino            *
*                                                    *
* this version adapted for Arduino                   *
*                                                    *
* (C) 2012,2013,2014                                 *
*                                                    *
* Jens Dalsgaard Nielsen <jdn@es.aau.dk>             *
* http://es.aau.dk/staff/jdn                         *
* Section of Automation & Control                    *
* Aalborg University,                                *
* Denmark                                            *
*                                                    *
* "THE BEER-WARE LICENSE" (frit efter PHK)           *
* <jdn@es.aau.dk> wrote this file. As long as you    *
* retain this notice you can do whatever you want    *
* with this stuff. If we meet some day, and you think*
* this stuff is worth it ...                         *
*  you can buy me a beer in return :-)               *
* or if you are real happy then ...                  *
* single malt will be well received :-)              *
*                                                    *
* Use it at your own risk - no warranty              *
*                                                    *
* tested with duemilanove w/328, uno R3,             *
* seeduino 1280 and mega2560                         *
*****************************************************/


#include "krnl.h"
#include <avr/wdt.h>
// CPU frequency - for adjusting delays
#if (F_CPU == 16000000)
#pragma message ("krnl detected 16 MHz" )
#else
#pragma message ("krnl detected 8 MHz")
#endif


#if (KRNL_VRS != 2001)
#error "KRNL VERSION NOT UPDATED in krnl.c /JDN"
#endif

#include <avr/interrupt.h>
#include <stdlib.h>

#if (KRNLTMR == 0)
// normally not goood bq of arduino sys timer so you wil get a compile error
// 8 bit timer !!!
#define KRNLTMRVECTOR TIMER0_OVF_vect
#define TCNTx TCNT0
#define TCCRxA TCCR0A
#define TCCRxB TCCR0B
#define TCNTx TCNT0
#define OCRxA OCR0A
#define TIMSKx TIMSK0
#define TOIEx TOIE0
#define PRESCALE 0x07
#define COUNTMAX 255
#define DIVV 15.625
#define DIVV8 7.812 

#elif (KRNLTMR == 1)

#define KRNLTMRVECTOR TIMER1_OVF_vect
#define TCNTx TCNT1
#define TCCRxA TCCR1A
#define TCCRxB TCCR1B
#define TCNTx TCNT1
#define OCRxA OCR1A
#define TIMSKx TIMSK1
#define TOIEx TOIE1
#define PRESCALE 0x04
#define COUNTMAX 65535
#define DIVV 62.5
#define DIVV8 31.25
 
#elif (KRNLTMR == 2)

// 8 bit timer !!!
#define KRNLTMRVECTOR TIMER2_OVF_vect
#define TCNTx TCNT2
#define TCCRxA TCCR2A
#define TCCRxB TCCR2B
#define TCNTx TCNT2
#define OCRxA OCR2A
#define TIMSKx TIMSK2
#define TOIEx TOIE2
#define PRESCALE 0x07
#define COUNTMAX 255
#define DIVV 15.625
#define DIVV8 7.812 

#elif (KRNLTMR == 3)

#define KRNLTMRVECTOR TIMER3_OVF_vect
#define TCNTx TCNT3
#define TCCRxA TCCR3A
#define TCCRxB TCCR3B
#define TCNTx TCNT3
#define OCRxA OCR3A
#define TIMSKx TIMSK3
#define TOIEx TOIE3
#define PRESCALE 0x04
#define COUNTMAX 65535
#define DIVV 62.5
#define DIVV8 31.25

#elif (KRNLTMR == 4)

#define KRNLTMRVECTOR TIMER4_OVF_vect
#define TCNTx TCNT4
#define TCCRxA TCCR4A
#define TCCRxB TCCR4B
#define TCNTx TCNT4
#define OCRxA OCR4A
#define TIMSKx TIMSK4
#define TOIEx TOIE4
#define PRESCALE 0x04
#define COUNTMAX 65535
#define DIVV 62.5
#define DIVV8 31.25

#elif (KRNLTMR == 5)

#define KRNLTMRVECTOR TIMER5_OVF_vect
#define TCNTx TCNT5
#define TCCRxA TCCR5A
#define TCCRxB TCCR5B
#define TCNTx TCNT5
#define OCRxA OCR5A
#define TIMSKx TIMSK5
#define TOIEx TOIE5
#define PRESCALE 0x04
#define COUNTMAX 65535
#define DIVV 62.5
#define DIVV8 31.25

#else

#pragma err "no valid tmr selected"

#endif

// can be 1,2,3,4,5,6
//----------------------------------------------------------------------------

struct k_t
        *task_pool, // array of descriptors for tasks
        *sem_pool,  // .. for semaphores
        AQ,       // Q head for active Q
        *pmain_el,  // procesdecriptor for main
        *pAQ,     // head of activeQ (AQ)
        *pDmy,    // ref to dummy task
        *pRun,    // who is running ?
        *pSleepSem; // one semaphor for all to sleep at

struct k_msg_t *send_pool;   // ptr to array for msg sem pool

int k_task, k_sem, k_msg; // how many di you request in k_init of descriptors ?
char nr_task = 0,nr_sem = 0,	nr_send = 0;	// counters for created KeRNeL items

// NASTY char dmy_stk[DMY_STK_SZ];

volatile char krnl_preempt_flag=1; //1: preempt, 0 : non preempt
volatile char k_running = 0,	k_err_cnt = 0;
volatile unsigned int tcntValue;	// counters for timer system
volatile int fakecnt, // counters for letting timer ISR go multipla faster than krnl timer
fakecnt_preset;

static volatile char stopp = 0;

unsigned long k_millis_counter=0;
unsigned int k_tick_size;

int tmr_indx; // for travelling Qs in tmr isr


/**
 * just for eating time
 * eatTime in msec's
 */
 char k_eat_time(unsigned int eatTime)
 {
    unsigned long l;
    // tested on uno for 5 msec and 500 msec
    // if you are preempted then ... :-(

    // quants in milli seconds
    // not 100% precise !!!
    l = eatTime;
#if (F_CPU == 16000000)
    l *=1323;
#elif (F_CPU == 8000000)
    l *=661;
#else
#error bad cpu frequency
#endif

    while (l--) {
        asm("nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t nop");
    }
}

//---QOPS---------------------------------------------------------------------

void
enQ (struct k_t *Q, struct k_t *el)
{
    el->next = Q;
    el->pred = Q->pred;
    Q->pred->next = el;
    Q->pred = el;
}

//----------------------------------------------------------------------------

struct k_t *
deQ (struct k_t *el)
{
    el->pred->next = el->next;
    el->next->pred = el->pred;

    return (el);
}

//----------------------------------------------------------------------------

void
prio_enQ (struct k_t *Q, struct k_t *el)
{
    char prio = el->prio;

    Q = Q->next;		// bq first elm is Q head itself

    while (Q->prio <= prio) { // find place before next with lower prio
        Q = Q->next;
    }

    el->next = Q;
    el->pred = Q->pred;
    Q->pred->next = el;
    Q->pred = el;
}

//---HW timer IRS-------------------------------------------------------------
//------------timer section---------------------------------------------------
/*
 * The KRNL Timer is driven by timer
 *
 *
 * Install the Interrupt Service Routine (ISR) for Timer2 overflow.
 * This is normally done by writing the address of the ISR in the
 * interrupt vector table but conveniently done by using ISR()
 *
 * Timer2 reload value, globally available
 */
 struct k_t *pE;


 ISR (KRNLTMRVECTOR, ISR_NAKED)
 {
    // no local vars ! I think
    PUSHREGS ();

    TCNTx = tcntValue;		// Reload the timer

    if (!k_running)  // obvious
        goto exitt;

    fakecnt--;   // for very slow k_start values bq timer cant run so slow (8 bit timers at least)
    if (0 < fakecnt)		// how often shall we run KeRNeL timer code ?
        goto exitt;

    fakecnt = fakecnt_preset;	// now it's time for doing RT stuff

    k_millis_counter+=k_tick_size; // my own millis counter

    // here you may maintain Arduinos msec defined in wiring.c as
    //unsigned long m  timer0_millis;
    // It looks maybe crazy to go through all semaphores and tasks
    // but
    // you may have 3-4 tasks and 3-6 semaphores in your code
    // so...
    // and - it's a good idea not to init krnl with more items (tasks/Sem/msg descriptors than needed)


    pE = sem_pool;		// Semaphore timer - check timers on semaphores - they may be cyclic

    for (tmr_indx = 0; tmr_indx < nr_sem; tmr_indx++) {
        if (0 < pE->cnt2) {                         // timer on semaphore ?
            pE->cnt2--;                               // yep  decrement it
            if (pE->cnt2 <= 0) {                      // timeout  ?
                pE->cnt2 = pE->cnt3;	                  // preset again - if cnt3 == 0 and >= 0 the rep timer
                ki_signal (pE);	                        //issue a signal to the semaphore
            }
        }
        pE++;
    }

    pE = task_pool;                               // Chk timers on tasks - they may be one shoot waiting

    for (tmr_indx = 0; tmr_indx < nr_task; tmr_indx++) {
        if (0 < pE->cnt2) {                        // timer active on task ?
            pE->cnt2--;                              // yep so let us do one down count
            if (pE->cnt2 <= 0) {                     // timeout ? ( == 0 )
                pE->cnt2 = -1;	                       // indicate timeout inis semQ
                prio_enQ (pAQ, deQ (pE));	             // to AQ
            }
        }
        pE++;
    }

    if (krnl_preempt_flag) {
      prio_enQ (pAQ, deQ (pRun));	// round robbin

      K_CHG_STAK ();
  }

  exitt:
  POPREGS ();
  RETI ();
}

//----------------------------------------------------------------------------
// inspired from ...
// http://arduinomega.blogspot.dk/2011/05/timer2-and-overflow-interrupt-lets-get.html
// Inspiration from  http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
//TIMSK2 &= ~(1 << TOIE2);  // Disable the timer overflow interrupt while we're configuring
//TCCR2B &= ~(1 << WGM22);
// JDN

//----------------------------------------------------------------------------
// Inspiration from "Multitasking on an AVR" by Richard Barry, March 2004
// avrfreaks.net
// and my old kernel from last century
// basic concept from my own very old kernels dated back bef millenium

void __attribute__ ((naked, noinline)) ki_task_shift (void)
{
    PUSHREGS ();		        // push task regs on stak so we are rdy to task shift
    K_CHG_STAK();
    POPREGS ();			        // restore regs
    RETI ();			        // and do a reti NB this also enables interrupt !!!
}

// HW_DEP_ENDE

//----------------------------------------------------------------------------

struct k_t *
k_crt_task (void (*pTask) (void), char prio, char *pStk, int stkSize)
{

    struct k_t *pT;
    int i;
    char *s;

    if (k_running)
        goto badexit;

    if ((prio <= 0 ) || (DMY_PRIO < prio)) {
        goto badexit;
    }

    if (k_task <= nr_task) { // no vacant elm in buffer
        goto badexit; 
    }

    pT = task_pool + nr_task;	// lets take a task descriptor
    pT->nr = nr_task;
    nr_task++;

    pT->cnt2 = 0;		// no time out running on you for the time being
    pT->cnt3 = 0;		// no time out semaphore 

    // HW_DEP_START
    // inspiration from http://dev.bertos.org/doxygen/frame_8h_source.html
    // and http://www.control.aau.dk/~jdn/kernels/krnl/
    // now we are going to precook stak

    pT->cnt1 = (int) (pStk);

    for (i = 0; i < stkSize; i++)	// put hash code on stak to be used by k_unused_stak()
        pStk[i] = STAK_HASH;

    // HW DEPENDENT PART
    s = pStk + stkSize - 1;	// now we point on top of stak
    *(s--) = 0x00;		    // 1 byte safety distance
    *(s--) = lo8 (pTask);	//  so top now holds address of function
    *(s--) = hi8 (pTask);	// which is code body for task

    // NB  NB 2560 use 3 byte for call/ret addresses the rest only 2
#if defined (__AVR_ATmega2560__)
    *(s--) = EIND;		// best guess : 3 byte addresses !!!
#endif

    *(s--) = 0x00;		// r1
    *(s--) = 0x00;		// r0
    *(s--) = 0x00;		// sreg

    //1280 and 2560 need to save rampz reg just in case
#if defined (__AVR_ATmega2560__) || defined (__AVR_ATmega1280__)
    *(s--) = RAMPZ;		// best guess
    *(s--) = EIND;		// best guess
#endif

    for (i = 0; i < 30; i++)	//r2-r31 = 30 regs
        *(s--) = 0x00;

    pT->sp_lo = lo8 (s);	// now we just need to save stakptr
    pT->sp_hi = hi8 (s);	// in thread descriptor

    // HW DEPENDENT PART - ENDE

    pT->prio = prio;		// maxv for holding org prio for inheritance
    pT->maxv = (int) prio;
    prio_enQ (pAQ, pT);		// and put task in active Q

    return (pT);

    badexit:
    k_err_cnt++;
    return(NULL);
}

//----------------------------------------------------------------------------

int
freeRam (void)
{
    extern int __heap_start, *__brkval;
    int v;

    return ((int) &v -
        (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

//----------------------------------------------------------------------------

int
k_sleep (int time)
{
    return k_wait (pSleepSem, time);
}

//----------------------------------------------------------------------------

int
k_unused_stak (struct k_t *t)
{
    int i = 0;
    char *pstk;

    if (t) // another task or yourself - NO CHK of validity !
        pstk = (char *) (t->cnt1);
    else
        pstk = (char *) (pRun->cnt1);

    DI ();
    while (*pstk == STAK_HASH) {
        pstk++;
        i++;
    }
    EI ();

    return (i);
}

//----------------------------------------------------------------------------
k_set_prio (char prio)
{

    if (!k_running)
        return (-1);

    DI ();

    if ((prio <= 0) || (DMY_PRIO <= prio)) {
        // not legal value my friend
        EI ();
        return (-2);
    }

    pRun->prio = prio;
    prio_enQ (pAQ, deQ (pRun));
    ki_task_shift ();

    EI ();

    return (0);
}

//----------------------------------------------------------------------------

struct k_t *
k_crt_sem (char init_val, int maxvalue)
{

    struct k_t *sem;

    if (k_running)
        return (NULL);

    if ((init_val < 0) || (maxvalue < 0)) {
        goto badexit;
    }

    if (k_sem <= nr_sem) {
        goto badexit;
    }

    sem = sem_pool + nr_sem;
    sem->nr = sem;
    nr_sem++;

    sem->cnt2 = 0;		// no timer running
    sem->next = sem->pred = sem;
    sem->prio = QHD_PRIO;
    sem->cnt1 = init_val;
    if (0 < maxvalue && 32000 > maxvalue)
        sem->maxv = maxvalue;
    else
        sem->maxv = SEM_MAX_DEFAULT;
    sem->clip = 0;
    return (sem);

    badexit:
    k_err_cnt++;

    return (NULL);
}

//----------------------------------------------------------------------------

int
k_set_sem_timer (struct k_t *sem, int val)
{

    // there is no k_stop_sem_timer fct just call with val== 0

    if (val < 0)
        return (-1);		// bad value

    DI ();
    sem->cnt2 = sem->cnt3 = val;	// if 0 then timer is not running - so
    EI ();

    return (0);
}

//----------------------------------------------------------------------------

int
ki_signal (struct k_t *sem)
{
	DI(); // just in case
    if (sem->maxv <= sem->cnt1) {
        if (32000 > sem->clip)
            sem->clip++;
        k_sem_clip(0);
        return (-1);
    }

    sem->cnt1++;		// Salute to Dijkstra

    if (sem->cnt1 <= 0) {
        sem->next->cnt2 = 0;	// return code == ok
        prio_enQ (pAQ, deQ (sem->next));
    }

    return (0);
}

//----------------------------------------------------------------------------

int
k_signal (struct k_t *sem)
{
    int res;

    DI ();

    res = ki_signal (sem);

    if (res == 0)
        ki_task_shift ();

    EI ();

    return (res);
}

//----------------------------------------------------------------------------

int
ki_nowait (struct k_t *sem)
{

    DI ();			// DI just to be sure
    if (0 < sem->cnt1) {
        //      lucky that we do not need to wait ?
        sem->cnt1--;		// Salute to Dijkstra
        return (0);
    } else
    return (-1);
}

//----------------------------------------------------------------------------

int
ki_wait (struct k_t *sem, int timeout)
{
// used by msg system
    DI ();

    if (0 < sem->cnt1) {
        //      lucky that we do not need to wait ?
        sem->cnt1--;		// Salute to Dijkstra
        return (0);
    }

    if (timeout == -1) {
        // no luck, dont want to wait so bye bye
        return (-2);
    }

    // from here we want to wait
    pRun->cnt2 = timeout;	// if 0 then wait forever

    if (timeout)
        pRun->cnt3 = (int) sem;	// nasty keep ref to semaphor
    //  so we can be removed if timeout occurs
    sem->cnt1--;		// Salute to Dijkstra

    enQ (sem, deQ (pRun));
    ki_task_shift ();		// call enables NOT interrupt on return
 	pRun->cnt3 = 0; // reset ref to timer semaphores
    return ((char) (pRun->cnt2));	// 0: ok, -1: timeout
}

//----------------------------------------------------------------------------

int
k_wait (struct k_t *sem, int timeout)
{

int retval;
    // copy of ki_wait just with EI()'s before leaving
    DI ();

    if (0 < sem->cnt1) {     // lucky that we do not need to wait ?
        sem->cnt1--;		       // Salute to Dijkstra
        EI ();
        return (0);
    }

    if (timeout == -1) {     // no luck, dont want to wait so bye

        EI ();
        return (-2);
    }

    // from here we have to wait
    pRun->cnt2 = timeout;	// if 0 then wait forever

    if (timeout)
        pRun->cnt3 = (int) sem;	// nasty keep ref to semaphore,
    //  so we can be removed if timeout occurs
    sem->cnt1--;		// Salute to Dijkstra

    enQ (sem, deQ (pRun));
    ki_task_shift ();		// call enables interrupt on return
    pRun->cnt3 = 0; // reset ref to timer semaphore
    retval =  pRun->cnt2;
    EI ();

    return  retval;	// 0: ok, -1: timeout
}

//----------------------------------------------------------------------------
int
k_wait_lost (struct k_t *sem, int timeout, int *lost)
{
    DI();
    if (lost != NULL) {
        *lost = sem->clip;
        sem->clip = 0;
    }
    return k_wait(sem,timeout);
}

//----------------------------------------------------------------------------
int
k_sem_signals_lost (struct k_t *sem)
{
    int x;
    DI();
    x = sem->clip;
    sem->clip = 0;
    EI();
    return x;
}

//----------------------------------------------------------------------------

int
ki_semval (struct k_t *sem)
{

    int i;

    DI (); // just to be sure
    // bq we ant it to be atomic
    i = sem->cnt1;
    //   EI (); NOPE dont enable ISR my friend

    return (i);
}

//----------------------------------------------------------------------------

char
ki_send (struct k_msg_t *pB, void *el)
{

    int i;
    char *pSrc, *pDst;

    if (pB->nr_el <= pB->cnt) {
        // room for a putting new msg in Q ?
        if (pB->lost_msg < 32000)
            pB->lost_msg++;
        return (-1);		// nope
    }

    pB->cnt++;

    pSrc = (char *) el;

    pB->w++;
    if (pB->nr_el <= pB->w)	// simple wrap around
        pB->w = 0;

    pDst = pB->pBuf + (pB->w * pB->el_size);	// calculate where we shall put msg in ringbuf

    for (i = 0; i < pB->el_size; i++) {
        // copy to Q
        *(pDst++) = *(pSrc++);
    }

    ki_signal (pB->sem);	// indicate a new msg is in Q

    return (0);
}

//----------------------------------------------------------------------------

char
k_send (struct k_msg_t *pB, void *el)
{

    char res;

    DI ();

    res = ki_send (pB, el);

    if (res == 0)
        ki_task_shift ();

    EI ();

    return (res);
}

//----------------------------------------------------------------------------

char
ki_receive (struct k_msg_t *pB, void *el, int *lost_msg)
{

    int i;
    char *pSrc, *pDst;

    // can be called from ISR bq no blocking
    DI ();			// just to be sure

    if (ki_nowait (pB->sem) == 0) {

        pDst = (char *) el;
        pB->r++;
        pB->cnt--;		// got one

        if (pB->nr_el <= pB->r)
            pB->r = 0;

        pSrc = pB->pBuf + pB->r * pB->el_size;

        for (i = 0; i < pB->el_size; i++) {
            *(pDst++) = *(pSrc++);
        }
        if (lost_msg) {
            *lost_msg = pB->lost_msg;
            pB->lost_msg = 0;
        }
        return (0);		// yes
    }

    return (-1);		// nothing for you my friend
}

//----------------------------------------------------------------------------

char
k_receive (struct k_msg_t *pB, void *el, int timeout, int *lost_msg)
{

    int i;
    char *pSrc, *pDst;

    DI ();

    if (ki_wait (pB->sem, timeout) == 0) {
        // ki_wait bq then intr is not enabled when coming back
        pDst = (char *) el;
        pB->r++;
        pB->cnt--;		// got one

        if (pB->nr_el <= pB->r)
            pB->r = 0;

        pSrc = pB->pBuf + pB->r * pB->el_size;

        for (i = 0; i < pB->el_size; i++) {
            *(pDst++) = *(pSrc++);
        }
        if (lost_msg) {
            *lost_msg = pB->lost_msg;
            pB->lost_msg = 0;
        }

        EI ();
        return (0);		// yes
    }

    EI ();
    return (-1);		// nothing for you my friend
}

//----------------------------------------------------------------------------

struct k_msg_t *
k_crt_send_Q (int nr_el, int el_size, void *pBuf)
{

    struct k_msg_t *pMsg;

    if (k_running)
        return (NULL);

    if (k_msg <= nr_send)
        goto errexit;

    if (k_sem <= nr_sem)
        goto errexit;

    pMsg = send_pool + nr_send;
    pMsg->nr = nr_send;
    nr_send++;

    pMsg->sem = k_crt_sem (0, nr_el);

    if (pMsg->sem == NULL)
        goto errexit;

    pMsg->pBuf = (char *) pBuf;
    pMsg->r = pMsg->w = -1;
    pMsg->el_size = el_size;
    pMsg->nr_el = nr_el;
    pMsg->lost_msg = 0;
    pMsg->cnt = 0;

    return (pMsg);

    errexit:
    k_err_cnt++;
    return (NULL);
}


//----------------------------------------------------------------------------

void
k_round_robbin (void)
{

    // reinsert running task in activeQ if round robbin is selected
    DI ();

    prio_enQ (pAQ, deQ (pRun));
    ki_task_shift ();

    EI ();
}

//---------------------------------------------------------------------fakecnt-------

void k_bugblink13(char blink)
{
    /*not impl anymore */
}

//----------------------------------------------------------------------------

/* NASTYvoid
dummy_task (void)
{
    while (1) { }
}
*/

//----------------------------------------------------------------------------

int
k_init (int nrTask, int nrSem, int nrMsg)
{

    if (k_running)  // are you a fool ???
        return (NULL);

    k_task = nrTask + 1;	// +1 due to dummy
    k_sem = nrSem + nrMsg + 1;	// due to that every msgQ has a builtin semaphore
    k_msg = nrMsg;

    task_pool = (struct k_t *) malloc (k_task * sizeof (struct k_t));
    sem_pool = (struct k_t *) malloc (k_sem * sizeof (struct k_t));
    send_pool = (struct k_msg_t *) malloc (k_msg * sizeof (struct k_msg_t));

    // we dont accept any errors
    if ((task_pool == NULL) || (sem_pool == NULL) || (send_pool == NULL)) {
        k_err_cnt++;
        goto leave;
    }

    // init AQ as empty double chained list
    pAQ = &AQ;
    pAQ->next = pAQ->pred = pAQ;
    pAQ->prio = QHD_PRIO;

    // JDN pDmy = k_crt_task (dummy_task, DMY_PRIO, dmy_stk, DMY_STK_SZ);
    pmain_el = task_pool;
    nr_task++;
    pmain_el->prio = DMY_PRIO;
    prio_enQ(pAQ,pmain_el); 

    pSleepSem = k_crt_sem (0, 2000);

    leave:
    return k_err_cnt;
}

//-------------------------------------------------------------------------------------------
int
k_start (int tm)
{
    /*
     48,88,168,328, 1280,2560
     timer 0 and 2 has same prescaler config:

        0 0 0 No clock source (Timer/Counter stopped).
        0 0 1 clk T2S /(No prescaling)
        0 1 0 clk T2S /8 (From prescaler)      2000000 intr/sec at 1 downcount
        0 1 1 clk T2S /32 (From prescaler)      500000 intr/sec ...
        1 0 0 clk T2S /64 (From prescaler)      250000
        1 0 1 clk T2S /128 (From prescaler)     125000
        1 1 0 clk T 2 S /256 (From prescaler)    62500
        1 1 1 clk T 2 S /1024 (From prescaler)   15625  eq 15.625 count down for 1 millisec so 255 counts ~= 80.32 milli sec timer

    timer 1(328+megas), 3,4,5(megas only)
        1280, 2560,2561 has same prescaler config :
        FOR 16 bits !
        prescaler in cs2 cs1 cs0
        0   0   0   none
        0   0   1   /1 == none
        0   1   0   /8     2000000 intr/sec
        0   1   1   /64     250000 intr/sec
        1   0   0   /256     62500 intr/sec
        1   0   1   /1024    15625 intr/sec
        16MHz Arduino -> 16000000/1024 =  15625 intr/second at one count
        16MHz Arduino -> 16000000/256  =  62500 ticks/second
        -------------------------/64   = 250000 ticks/second !

        NB 16 bit counter so values >= 65535 is not working
        ***************************************************************************************/

    // will not start if errors during initialization
        if (k_err_cnt)
            return -k_err_cnt;
    // boundary check
    if (tm <= 0)
        return -555;
    else if (10 >= tm) {
        fakecnt = fakecnt_preset=0; // on duty for every interrupt
    } else if ( (tm <= 10000) &&  (10*(tm/10) == tm) ) { // 20,30,40,50,...,10000
        fakecnt_preset = fakecnt = tm/10;
        tm = 10;
    } else {
        return -666;
    }
    
    DI (); // silencio
    k_tick_size = tm;
#if defined(__AVR_ATmega32U4__)
    // 32u4 have no intern/extern clock source register
#else
    // should be default ASSR &= ~(1 << AS2);	// Select clock source: internal I/O clock 32u4 does not have this facility
#endif

    TCCRxA = 0;
    TCCRxB = PRESCALE; // atm328s  2560,...

if (F_CPU == 16000000L)
    tcntValue = COUNTMAX  - tm*DIVV;
else
    tcntValue = COUNTMAX  - tm*DIVV8;  // 8 Mhz wwe assume

    TCNTx = tcntValue;

    //  let us start the show
    TIMSKx |= (1 << TOIEx); // enable interrupt

    pRun = pmain_el;		// just for ki_task_shift
    k_running = 1;

    DI ();
    ki_task_shift ();		// bye bye from here
    EI ();
    while (!stopp);

    return (pmain_el->cnt1);	// haps from pocket from kstop
}

int k_stop(int exitVal)
{
// DANGEROUS - handle with care - no isr timer control etc etc 
// I WILL NEVER USE IT
    DI(); // silencio
    if (!k_running) {
        EI();
        return -1;
    }

    pmain_el->cnt1 = exitVal; // transfer in pocket
    //NASTY
    // stop tick timer isr
    TIMSKx &= ~(1 << TOIEx);

    stopp = 1;
    // back to main
    AQ.next = pmain_el; // we will be the next BRUTAL WAY TO DO IT NASTY
    ki_task_shift();
    while (1); // you will never come here
}

void k_reset()
{
    DI();
    wdt_enable(WDTO_15MS);
    while(1);
}
//-------------------------------------------------------------------------------------------

unsigned long k_millis(void)
{
unsigned long l;
 
    DI();
    l = k_millis_counter;
    EI();
    return l;
}

//-------------------------------------------------------------------------------------------

int k_tmrInfo(void)
{
    return (KRNLTMR);
}

//-------------------------------------------------------------------------------------------

char k_set_preempt(char on)
{
 if (on == 0 || on == 1)
   krnl_preempt_flag = on;
return krnl_preempt_flag;
}

//-------------------------------------------------------------------------------------------

char k_get_preempt(void)
{
  return krnl_preempt_flag;
}


void __attribute__ ((weak)) k_sem_clip(int nr)
{

}
void __attribute__ ((weak)) k_send_Q_clip(int nr)
{

}


/* EOF - JDN */

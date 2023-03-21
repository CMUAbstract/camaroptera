#include <msp430.h>
#include <stdlib.h>
#include <stdbool.h>

#include "periph.h"

#include "sleep.h"

#define TIMER_NAME_INNER_INNER(type, id) type ## id
#define TIMER_NAME_INNER(type, id) TIMER_NAME_INNER_INNER(type, id)
#define SLEEP_TIMER TIMER_NAME_INNER(LIBMSP_SLEEP_TIMER_TYPE, LIBMSP_SLEEP_TIMER_IDX)

static msp_alarm_cb_t *alarm_callback;

#ifdef LIBMSP_SLEEP_TIMER_TICKS
static unsigned requested_ticks;
static unsigned ticks;
#endif // LIBMSP_SLEEP_TIMER_TICKS

#ifdef LIBMSP_SLEEP_TIMER_TICKS
static void setup_timer(unsigned cycles, unsigned ticks)
#else
static void setup_timer(unsigned cycles)
#endif // LIBMSP_SLEEP_TIMER_TICKS
{
    TIMER(SLEEP_TIMER, CTL) = TIMER_CLK_SOURCE_BITS(LIBMSP_SLEEP_TIMER_TYPE, LIBMSP_SLEEP_TIMER_CLK) |
                              TIMER_DIV_BITS(DIV_LIBMSP_SLEEP_TIMER) |
                              TIMER_CLR(LIBMSP_SLEEP_TIMER_TYPE);
    TIMER(SLEEP_TIMER, EX0) = TIMER_DIVEX_BITS(LIBMSP_SLEEP_TIMER_TYPE, DIVEX_LIBMSP_SLEEP_TIMER);

    TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCR) = cycles;
    TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCTL) &= ~CCIFG;
    TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCTL) |= CCIE;

#ifdef LIBMSP_SLEEP_TIMER_TICKS
    requested_ticks = ticks;
    ticks = 0;
#endif // LIBMSP_SLEEP_TIMER_TICKS
}

#ifdef LIBMSP_SLEEP_TIMER_TICKS
void msp_sleep(unsigned cycles, unsigned ticks)
#else
void msp_sleep(unsigned cycles)
#endif // LIBMSP_SLEEP_TIMER_TICKS
{
#ifdef LIBMSP_SLEEP_TIMER_TICKS
    setup_timer(cycles, ticks);
#else
    setup_timer(cycles);
#endif // LIBMSP_SLEEP_TIMER_TICKS

    alarm_callback = NULL;

#ifdef LIBMSP_SLEEP
    // The int disable/enable implement the classic check-and-sleep pattern
    __disable_interrupt();
    TIMER(SLEEP_TIMER, CTL) |= MC_1; // start timer

    while (TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCTL) & CCIE) { // might not be our ISR
        __bis_SR_register(LPM0_bits + GIE); // enable interrupt and go to sleep
        __disable_interrupt();
    }
    __enable_interrupt();
#else // ! LIBMSP_SLEEP (polling)
    TIMER(SLEEP_TIMER, CTL) |= MC_1; // start timer
    while (TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCTL) & CCIE);
#endif // ! LIBMSP_SLEEP (polling)
}

#ifdef LIBMSP_SLEEP_TIMER_TICKS
void msp_alarm(unsigned cycles, unsigned ticks, msp_alarm_cb_t *cb)
#else
void msp_alarm(unsigned cycles, msp_alarm_cb_t *cb)
#endif // LIBMSP_SLEEP_TIMER_TICKS
{
#ifdef LIBMSP_SLEEP_TIMER_TICKS
    setup_timer(cycles, ticks);
#else
    setup_timer(cycles);
#endif // LIBMSP_SLEEP_TIMER_TICKS

    alarm_callback = cb;
    TIMER(SLEEP_TIMER, CTL) |= MC_1; // start timer
}

// We declare the ISR in the library, which disallows sequential multiplexing of this timer.
// Note: there's no concurrent multiplexing of the timer (at least not in this
// simple design), so the ISR can be hidden in the library.
ISR(TIMER_VECTOR(LIBMSP_SLEEP_TIMER_TYPE, LIBMSP_SLEEP_TIMER_IDX, LIBMSP_SLEEP_TIMER_CC))
{
    // CCIFG automatically reset

#ifdef LIBMSP_SLEEP_TIMER_TICKS
    if (++ticks == requested_ticks) {
#endif // LIBMSP_SLEEP_TIMER_TICKS
        TIMER(SLEEP_TIMER, CTL) &= ~MC_3; // stop timer
        TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCTL) &= ~CCIE; // used as flag that ISR ran
        TIMER_CC(SLEEP_TIMER, LIBMSP_SLEEP_TIMER_CC, CCTL) &= ~CCIFG; // clear flag, just to be sure

        bool wakeup = true;
        if (alarm_callback) { // request was an alarm request
            msp_alarm_action_t act = alarm_callback();
            wakeup = act & MSP_ALARM_ACTION_WAKEUP;
        }

        if (wakeup) {
#ifdef LIBMSP_SLEEP
            __bic_SR_register_on_exit(LPM4_bits); // exit sleep
#endif // LIBMSP_SLEEP
        }
#ifdef LIBMSP_SLEEP_TIMER_TICKS
    } // else: we just incremented the tick count
#endif // LIBMSP_SLEEP_TIMER_TICKS
}

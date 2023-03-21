#ifndef LIBMSP_SLEEP_H
#define LIBMSP_SLEEP_H

// Go to sleep and set a timer to wake up in (ACLK) cycles
// On MSP430FR5949 the default ACLK is VLOCLK at ~10kHz.
#ifdef LIBMSP_SLEEP_TIMER_TICKS
void msp_sleep(unsigned cycles, unsigned ticks);
#else
void msp_sleep(unsigned cycles);
#endif // LIBMSP_SLEEP_TIMER_TICKS

typedef enum {
    MSP_ALARM_ACTION_WAKEUP  = 0x1,
} msp_alarm_action_t; // bitmask

typedef msp_alarm_action_t (msp_alarm_cb_t)(void); // returns whether to wakeup MCU after ISR

#ifdef LIBMSP_SLEEP_TIMER_TICKS
void msp_alarm(unsigned cycles, unsigned ticks, msp_alarm_cb_t *cb);
#else
void msp_alarm(unsigned cycles, msp_alarm_cb_t *cb);
#endif // LIBMSP_SLEEP_TIMER_TICKS

#endif // LIBMSP_SLEEP_H

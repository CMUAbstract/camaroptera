#include <msp430.h>

#include "watchdog.h"

void msp_watchdog_enable(uint8_t bits)
{
    WDTCTL = WDTPW | WDTCNTCL | bits;
}

void msp_watchdog_disable()
{
     WDTCTL = WDTPW + WDTHOLD;
}

void msp_watchdog_kick()
{
    WDTCTL = WDTPW | ((WDTCTL | WDTCNTCL) & 0x00ff);
}

void msp_watchdog_hold()
{
    WDTCTL = WDTPW | ((WDTCTL | WDTHOLD) & 0x00ff);
}

void msp_watchdog_release()
{
    WDTCTL = WDTPW | ((WDTCTL & ~WDTHOLD) & 0x00ff);
}

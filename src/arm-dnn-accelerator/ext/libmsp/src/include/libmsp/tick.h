#ifndef LIBMSP_TICK_H
#define LIBMSP_TICK_H

#include <stdint.h>

void msp_tick_start();
void msp_tick_stop();
uint32_t msp_ticks();

#endif // LIBMSP_TICK_H

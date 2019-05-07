#include <stdio.h>
#include <stdint.h>
#include <msp430.h>

#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <libfixed/fixed.h>
#include <libio/console.h>

//#include "svm.h"
//#include "input.h"

int main( int argc, char * argv[] ){

	msp_watchdog_disable();
	msp_gpio_unlock();

	msp_clock_setup();
	INIT_CONSOLE();

	uint8_t input[15] = [ 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4];

	uint16_t freq_array[2][256];
	uint32_t node_parent[512 ];

	uint16_t i;

	for( i = 0; i < 15; i++ )
		freq_array[input[i]] ++;

	return 0;
	}

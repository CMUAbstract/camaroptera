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


	fixed a = F_LIT(161.37);
	fixed c = F_LIT(0.8);
	fixed d = F_ADD(a, c);
	double x, y;
	uint16_t b = 0;
	d = F_ROUND(a);
	b = (uint16_t)F_TO_FLOAT(a);
	b = b%20;
	c = F_DIV( F_LIT(b), F_LIT(20));
	d = F_MUL( F_LIT(10.5), c);
	x = -11.5;
	y = x + 180.0;
	a = F_LIT( y);


	PRINTF("%u | %u | %u | %u\n\r", a, c, d, b);


	return 0;
	}

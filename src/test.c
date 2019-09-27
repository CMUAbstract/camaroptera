#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>
#include <libmsp/mem.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <libmspdriver/aes256.h>

uint16_t AES_baseAddr = 0x09C0;
const uint8_t encryptionKey[4] = {0x01, 0x02, 0x03, 0x04};
const uint8_t input_data[16] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
uint8_t output_data[16];

void main(){
	msp_watchdog_disable();
	msp_gpio_unlock();
	msp_clock_setup();
	INIT_CONSOLE();

	if( AES256_setCipherKey( AES_baseAddr, encryptionKey, 128 ) )
		PRINTF("Successfully Set AES encryption key\r\n");

	AES256_encryptData( AES_baseAddr, input_data, output_data );
	
	for( int i = 0; i < 16; i++ )
		PRINTF("Byte %i: %i\r\n", i, output_data[i]);
	
}

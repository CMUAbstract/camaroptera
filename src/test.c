#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <libio/console.h>
#include <libmsp/mem.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <libmspdriver/aes256.h>

#include <liblora/spi.h>
#include <liblora/sx1276.h>
#include <liblora/sx1276regs-fsk.h>
#include <liblora/sx1276regs-lora.h>

#define RF_FREQUENCY   915000000 // Hz

#define FSK_FDEV                          25e3      // Hz
#define FSK_DATARATE                      50e3      // bps
#define FSK_BANDWIDTH                     50e3      // Hz
#define FSK_AFC_BANDWIDTH                 83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH               5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON         false

#define LORA_BANDWIDTH                              2        // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       7        // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                  3500
#define TX_OUTPUT_POWER										17        // dBm
#define BUFFER_SIZE                       256 // Define the payload size here

#define __ro_hifram __attribute__((section(".upper.rodata")))

void camaroptera_init_lora();
void OnTxDone();
bool LoRaMac_EncryptPayload( uint8_t *data_buffer, uint16_t size, uint8_t *encoded_data_buffer, uint8_t dir, uint32_t deviceAddress, uint32_t sequenceCounter);

uint16_t AES_baseAddr = 0x09C0;
const uint8_t encryptionKey[16] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
const uint8_t input_data[16] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
uint8_t output_data[16];

// LoRaMac Definitions
uint8_t ablock[16] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t sblock[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t LoRaMacAppSKey[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

const uint32_t LoRaMacDeviceAddress = (uint32_t) 0x12345678; 

uint16_t upLinkCounter = 0;

__ro_hifram static radio_events_t radio_events;

int main(){
	msp_watchdog_disable();
	msp_gpio_unlock();
	msp_clock_setup();
	INIT_CONSOLE();

	P8OUT &= ~BIT1;
	P8DIR |= BIT1;

	/*
	if( AES256_setCipherKey( AES_baseAddr, encryptionKey, 128 ) )
		PRINTF("Successfully Set AES encryption key\r\n");
	
	P8OUT ^= BIT1;
	AES256_encryptData( AES_baseAddr, input_data, output_data );
	P8OUT ^= BIT1;
	*/

	uint8_t input_data = 0xAB;
	uint8_t result;
	LoRaMac_EncryptPayload( &input_data, 1, &result, 0x00, LoRaMacDeviceAddress, upLinkCounter );

	PRINTF("Encryption Result: %x\r\n", result);

	//for( int i = 0; i < 16; i++ )
		//PRINTF("Byte %i: %x\r\n", i, output_data[i]);
				
	//spi_init();
	//camaroptera_init_lora();
	//sx1276_send( radio_buffer,  last_packet_size);
	
	return 0;
}

bool LoRaMac_EncryptPayload( uint8_t *data_buffer, uint16_t size, uint8_t *encoded_data_buffer, uint8_t dir, uint32_t deviceAddress, uint32_t sequenceCounter){
	
	uint16_t i;
	uint8_t bufferIndex = 0;
	uint16_t count = 1;

	if( AES256_setCipherKey( AES_baseAddr, LoRaMacAppSKey, 128 ) )
		PRINTF("Successfully set AES encryption key\r\n");
	else{
		PRINTF("Could not set AES encryption key. Exiting. . .\r\n");
		return false;
		}
	
	ablock[5] = dir;

	ablock[6] = ( deviceAddress ) & 0xFF;
	ablock[7] = ( deviceAddress >> 8 ) & 0xFF;
	ablock[8] = ( deviceAddress >> 16 ) & 0xFF;
	ablock[9] = ( deviceAddress >> 24 ) & 0xFF;
	
	ablock[10] = ( sequenceCounter ) & 0xFF;
	ablock[11] = ( sequenceCounter >> 8 ) & 0xFF;
	ablock[12] = ( sequenceCounter >> 16 ) & 0xFF;
	ablock[13] = ( sequenceCounter >> 24 ) & 0xFF;
	
	while( size >= 16 ){
		ablock[15] = ( ( count ) & 0xFF );
		count++; 
		AES256_encryptData( AES_baseAddr, ablock, sblock );
		
		for( i = 0; i < 16; i++ )
			encoded_data_buffer[bufferIndex+i] = data_buffer[bufferIndex+i] ^ sblock[i];

		size -= 16;
		bufferIndex += 16;
		}
	
	if( size > 0 ){
		ablock[15] = ( ( count ) & 0xFF );
		AES256_encryptData( AES_baseAddr, ablock, sblock );
		
		for( i = 0; i < size; i++ )
			encoded_data_buffer[bufferIndex+i] = data_buffer[bufferIndex+i] ^ sblock[i];
		}
	
	return true;
	}

void OnTxDone() {
	//tx_packet_index ++;
	//sent_packet_count ++;
	//frame_track += 253;
}

void camaroptera_init_lora() {
  radio_events.TxDone = OnTxDone;
  //radio_events.RxDone = OnRxDone;
  //radio_events.TxTimeout = OnTxTimeout;
  //radio_events.RxTimeout = OnRxTimeout;
  //radio_events.RxError = OnRxError;

  sx1276_init(radio_events);
  sx1276_set_channel(RF_FREQUENCY);

	sx1276_set_txconfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 2000);
}


#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

#define RF_FREQUENCY   903000000 // Hz

#define FSK_FDEV                          25e3      // Hz
#define FSK_DATARATE                      50e3      // bps
#define FSK_BANDWIDTH                     50e3      // Hz
#define FSK_AFC_BANDWIDTH                 83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH               5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON         false

#define LORA_BANDWIDTH                              2        // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       8        // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                  3500
#define TX_OUTPUT_POWER										14        // dBm
#define BUFFER_SIZE                       256 // Define the payload size here

#define __ro_hifram __attribute__((section(".upper.rodata")))

void camaroptera_init_lora();
void OnTxDone();

uint16_t AES_baseAddr = 0x09C0;
__ro_hifram uint8_t radio_buffer[BUFFER_SIZE];

// ---- START LORAMAC STUFF ----
// LoRaMac Definitions

#define LORAMAC_MIC_BLOCK_SIZE                   16

#define COMPUTE_MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )

#define LSHIFT(v, r) do {                                       \
  int32_t i;                                                  \
	for (i = 0; i < 15; i++)                                \
		(r)[i] = (v)[i] << 1 | (v)[i + 1] >> 7;         \
	(r)[15] = (v)[15] << 1;                                 \
} while (0)
																									     
#define XOR(v, r) do {                                          \
	int32_t i;                                                  \
	for (i = 0; i < 16; i++)     \
		(r)[i] = (r)[i] ^ (v)[i]; \
} while (0) \

typedef struct aes_cmac_ctx{
	uint8_t X[16];
	uint8_t M_last[16];
	uint32_t M_n;
}aes_cmac_ctx;

aes_cmac_ctx AesCmacCtx;

uint8_t aBlock[16] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t sBlock[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t MICBlock[16] = { 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t LoRaMacAppSKey[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
const uint8_t LoRaMacNwkSKey[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

const uint32_t LoRaMacDeviceAddress = (uint32_t) 0x12345678; 

uint16_t upLinkCounter = 0;

// LoRaMac Function Declarations
bool LoRaMac_PrepareFrame( uint8_t *data_buffer, uint32_t deviceAddress, uint32_t sequenceCounter, uint8_t fport, uint8_t *encoded_payload, uint16_t payload_size );

bool LoRaMac_EncryptPayload( uint8_t *data_buffer, uint16_t size, uint8_t *encoded_data_buffer, uint8_t dir, uint32_t deviceAddress, uint32_t sequenceCounter);

bool LoRaMac_ComputeMIC( uint8_t *data_buffer, uint16_t size, uint32_t *mic, uint8_t dir, uint32_t deviceAddress, uint32_t sequenceCounter );

void AES_CMAC_Init( aes_cmac_ctx *ctx );
void AES_CMAC_Update( aes_cmac_ctx *ctx, uint8_t *data_buffer, uint32_t len );
void AES_CMAC_Final( uint8_t *encoded_data_buffer, aes_cmac_ctx *ctx );

// ---- END LORAMAC STUFF ----

__ro_hifram static radio_events_t radio_events;

int main(){
	msp_watchdog_disable();
	msp_gpio_unlock();
	msp_clock_setup();
	INIT_CONSOLE();

	P8OUT &= ~BIT1;
	P8DIR |= BIT1;
	
	while(1){
		P8OUT ^= BIT1;
		__delay_cycles(16000000);
	}


	uint8_t input_payload[242];
	uint8_t encoded_payload[242];
	uint32_t mic;
	uint8_t final_frame[255];

	uint16_t i, payload_size, frame_size;
	
	payload_size = 5;
	frame_size = payload_size + 9;

	for( i = 0; i < payload_size; i++ )
		input_payload[i] = i % 255;

	P8OUT ^= BIT1;
	LoRaMac_EncryptPayload( input_payload, payload_size, encoded_payload, 0x00, LoRaMacDeviceAddress, upLinkCounter );
	P8OUT ^= BIT1;

	PRINTF("Encryption Result: \r\n");
	for( i = 0; i < payload_size; i++ )
		PRINTF("%x | ", encoded_payload[i]);
	PRINTF("\r\n");

	LoRaMac_PrepareFrame( final_frame, LoRaMacDeviceAddress, upLinkCounter, 0x0F, encoded_payload, payload_size );
	
	PRINTF("Final Frame: \r\n");
	for( i = 0; i < frame_size; i++ )
		PRINTF("%x | ", final_frame[i]);
	PRINTF("\r\n");
	
	P8OUT ^= BIT1;
	LoRaMac_ComputeMIC( final_frame, frame_size, &mic, 0x00, LoRaMacDeviceAddress, upLinkCounter );
	P8OUT ^= BIT1;

	PRINTF("MIC Computation Result:\r\n");
	
	for( i = 0; i < 4; i++ ){
		PRINTF("%x | ", (mic >> (i*8) ) & 0xFF);
		final_frame[frame_size + i] = (mic >> (i*8) ) & 0xFF;
	}
	PRINTF("\r\n");
	
	frame_size += 4;
	// Lorawan kept intact till here
	
	// Remove for loop for lorawan
	final_frame[0] = 0xDF;
	final_frame[1] = 0x09;
	final_frame[2] = 0x01;
	final_frame[3] = 0x05;
	final_frame[4] = 0x01;
	for( i = 5; i < 255; i++ ){
		final_frame[i] = i;
	}

#ifdef OLD_PINS
	PRINTF("OLD PINS!!\r\n");
	P4DIR |= BIT7;
	P4OUT |= BIT7;
#else
	PRINTF("NEW PINS!!\r\n");
	P4DIR |= BIT4;
	P4OUT |= BIT4;
#endif

	spi_init();
	camaroptera_init_lora();
	//sx1276_send( final_frame, frame_size ); // For lorawan
	sx1276_send( final_frame, 255 );
	__bis_SR_register(LPM4_bits+GIE);
	sx1276_on_dio0irq();
	
#ifdef OLD_PINS
	P5OUT &= ~BIT3;
	P1OUT &= ~BIT4;
	P4OUT &= ~BIT7;
#else
	P4OUT &= ~BIT1;
	P4OUT &= ~BIT2;
	P4OUT &= ~BIT4;
#endif

	PRINTF("Completed Transmission\r\n");

	return 0;
}

bool LoRaMac_PrepareFrame( uint8_t *data_buffer, uint32_t deviceAddress, uint32_t sequenceCounter, uint8_t fport, uint8_t *encoded_payload, uint16_t payload_size ){

	uint16_t i;

	if( payload_size < 0 || payload_size > 242 ){
		PRINTF("Invalid payload size\r\n");
		return false;
		}

	data_buffer[0] = 0x40; // MHDR: MType(3bits)=010 | RFU(3bits)=xxx | Major(2bits)=00
	
	// Append Device Address
	data_buffer[1] = ( deviceAddress ) & 0xFF;
	data_buffer[2] = ( deviceAddress >> 8 ) & 0xFF;
	data_buffer[3] = ( deviceAddress >> 16 ) & 0xFF;
	data_buffer[4] = ( deviceAddress >> 24 ) & 0xFF;

	data_buffer[5] = 0x00; // MACPayload: FHDR: FCtl: ADR|ADRAckReq|Ack|ClassB|FOpts(4bits)
	
	// MACPayload: FHDR: FCnt: sequenceCounter
	data_buffer[6] = ( sequenceCounter ) & 0xFF;
	data_buffer[7] = ( sequenceCounter >> 8 ) & 0xFF;

	data_buffer[8] = fport & 0xFF; // FPort: 0-Use NwkSKey for payload encr, 1..255-Use AppSKey for payload encr

	for( i = 0; i < payload_size ; i++ )
		data_buffer[i+9] = encoded_payload[i];
	

	return true;

	}

bool LoRaMac_EncryptPayload( uint8_t *data_buffer, uint16_t size, uint8_t *encoded_data_buffer, uint8_t dir, uint32_t deviceAddress, uint32_t sequenceCounter){
	
	uint16_t i;
	uint8_t bufferIndex = 0;
	uint16_t count = 1;

	if( AES256_setCipherKey( AES_baseAddr, LoRaMacAppSKey, 128 ) )
		PRINTF("Successfully set AES encryption key - LoRaMacAppSKey.\r\n");
	else{
		PRINTF("Could not set AES encryption key - LoRaMacAppSKey. Exiting. . .\r\n");
		return false;
		}
	
	aBlock[5] = dir;

	aBlock[6] = ( deviceAddress ) & 0xFF;
	aBlock[7] = ( deviceAddress >> 8 ) & 0xFF;
	aBlock[8] = ( deviceAddress >> 16 ) & 0xFF;
	aBlock[9] = ( deviceAddress >> 24 ) & 0xFF;
	
	aBlock[10] = ( sequenceCounter ) & 0xFF;
	aBlock[11] = ( sequenceCounter >> 8 ) & 0xFF;
	aBlock[12] = ( sequenceCounter >> 16 ) & 0xFF;
	aBlock[13] = ( sequenceCounter >> 24 ) & 0xFF;
	
	while( size >= 16 ){
		aBlock[15] = ( ( count ) & 0xFF );
		count++; 
		AES256_encryptData( AES_baseAddr, aBlock, sBlock );
		
		for( i = 0; i < 16; i++ )
			encoded_data_buffer[bufferIndex+i] = data_buffer[bufferIndex+i] ^ sBlock[i];

		size -= 16;
		bufferIndex += 16;
		}
	
	if( size > 0 ){
		aBlock[15] = ( ( count ) & 0xFF );
		AES256_encryptData( AES_baseAddr, aBlock, sBlock );
		
		for( i = 0; i < size; i++ )
			encoded_data_buffer[bufferIndex+i] = data_buffer[bufferIndex+i] ^ sBlock[i];
		}
	
	return true;
	}

bool LoRaMac_ComputeMIC( uint8_t *data_buffer, uint16_t size, uint32_t *mic, uint8_t dir, uint32_t deviceAddress, uint32_t sequenceCounter ){
	
	uint8_t Mic[16];
	
	MICBlock[5] = dir;
	
	MICBlock[6] = ( deviceAddress ) & 0xFF;
	MICBlock[7] = ( deviceAddress >> 8 ) & 0xFF;
	MICBlock[8] = ( deviceAddress >> 16 ) & 0xFF;
	MICBlock[9] = ( deviceAddress >> 24 ) & 0xFF;
	
	MICBlock[10] = ( sequenceCounter ) & 0xFF;
	MICBlock[11] = ( sequenceCounter >> 8 ) & 0xFF;
	MICBlock[12] = ( sequenceCounter >> 16 ) & 0xFF;
	MICBlock[13] = ( sequenceCounter >> 24 ) & 0xFF;

	MICBlock[15] = ( size ) & 0xFF;

	AES_CMAC_Init( &AesCmacCtx );

	if( AES256_setCipherKey( AES_baseAddr, LoRaMacNwkSKey, 128 ) )
		PRINTF("Successfully set AES encryption key - LoRaMacNwkSKey\r\n");
	else{
		PRINTF("Could not set AES encryption key - LoRaMacNwkSKey. Exiting. . .\r\n");
		return false;
		}
	
	AES_CMAC_Update( &AesCmacCtx, MICBlock, LORAMAC_MIC_BLOCK_SIZE );

	AES_CMAC_Update( &AesCmacCtx, data_buffer, size & 0xFF );

	AES_CMAC_Final( Mic, &AesCmacCtx );
	
	PRINTF("MICBlock: ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", MICBlock[i]);
	PRINTF("\r\n");

	*mic = ( uint32_t )( ( uint32_t )Mic[3] << 24 | ( uint32_t )Mic[2] << 16 | ( uint32_t )Mic[1] << 8 | ( uint32_t )Mic[0] );

	return true;
	}

void AES_CMAC_Init( aes_cmac_ctx *ctx ){
	
	memset( ctx->X, 0, sizeof(ctx->X) );
	ctx->M_n = 0;
	
	}

void AES_CMAC_Update( aes_cmac_ctx *ctx, uint8_t *data_buffer, uint32_t len ){
	
	uint32_t mlen;
	uint8_t in[16];
	uint8_t temp[16];

	PRINTF("M_n: %i\r\n", ctx->M_n);
	PRINTF("M_last: ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", ctx->M_last[i]);
	PRINTF("\r\n");
	PRINTF("X: ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", ctx->X[i]);
	PRINTF("\r\n");

	if( ctx->M_n > 0 ){
		mlen = COMPUTE_MIN( 16-ctx->M_n, len );
		memcpy( ctx->M_last + ctx->M_n, data_buffer, mlen );
		ctx->M_n += mlen;
		if( ctx->M_n < 16 || len == mlen )
			return;
		XOR( ctx->M_last, ctx->X );
		AES256_encryptData( AES_baseAddr, ctx->X, ctx->X );
		data_buffer += mlen;
		len -= mlen;
		}
	
	while( len > 16 ){ 	// Not last block
		XOR( data_buffer, ctx->X );
		memcpy( in, &ctx->X[0], 16 );
		AES256_encryptData( AES_baseAddr, in, in );
		memcpy( &ctx->X[0], in, 16 );
		data_buffer += 16;
		len -= 16;
		}
	
	// Potential last block, save it
	memcpy( ctx->M_last, data_buffer, len );
	ctx->M_n = len;
	
	}

void AES_CMAC_Final( uint8_t *encoded_data_buffer, aes_cmac_ctx *ctx ){
	
	uint8_t K[16];
	uint8_t in[16];
	
	PRINTF("==================\r\n");
	PRINTF("X: ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", ctx->X[i]);
	PRINTF("\r\n");

	// Generate subkey K1
	memset( K, '\0', 16 );
	AES256_encryptData( AES_baseAddr, K, K );
	
	PRINTF("K1: ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", K[i]);
	PRINTF("\r\n");
	
	if( K[0] & 0x80 ){
		LSHIFT( K, K );
		K[15] ^= 0x87;
		}
	else
		LSHIFT( K, K );

	if( ctx->M_n == 16 ) 		// Last Block was a complete block
		XOR( K, ctx->M_last );
	else{
		// Generate subkey K2
		if( K[0] & 0x80 ){
			LSHIFT( K, K );
			K[15] ^= 0x87;
			}
		else
			LSHIFT( K, K );
			
			PRINTF("K2: ");
			for(int i=0; i<16; i++)
				PRINTF("%x | ", K[i]);
			PRINTF("\r\n");
		
		// Padding(M_last)
		ctx->M_last[ctx->M_n] = 0x80;
		while( ++ctx->M_n < 16 )
			ctx->M_last[ctx->M_n] = 0;
		XOR( K, ctx->M_last );
		}

	XOR( ctx->M_last, ctx->X );
	memcpy( in, &ctx->X[0], 16 );
	AES256_encryptData( AES_baseAddr, in, encoded_data_buffer );
	PRINTF("in(final): ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", in[i]);
	PRINTF("\r\n");
	PRINTF("encoded_data_buffer(final): ");
	for(int i=0; i<16; i++)
		PRINTF("%x | ", encoded_data_buffer[i]);
	PRINTF("\r\n");
	
	memset( K, 0, sizeof(K) );

	PRINTF("==================\r\n");
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
 
 	uint64_t freq = RF_FREQUENCY;
	if( freq == 915000000 )
		PRINTF("TX FREQ = 915 MHz\r\n");
	else if( freq == 903000000 )
		PRINTF("TX FREQ = 903 MHz\r\n");

	PRINTF("LORA BANDWIDTH = %u\r\n", LORA_BANDWIDTH);
	PRINTF("LORA SPREADING FACTOR = %u\r\n", LORA_SPREADING_FACTOR);

	sx1276_set_channel(RF_FREQUENCY);
	
	uint8_t temp = sx1276_read(REG_FRFMSB);

	PRINTF("Value Read for frmsb= %x\r\n", temp);
	
	sx1276_set_txconfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 2000);
}


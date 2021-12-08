#ifndef CAMAROPTERA
	#define CAMAROPTERA
#endif

#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>

#include <libmsp/mem.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <liblora/mcu.h>
#include <liblora/spi.h>
#include <liblora/sx1276.h>
#include <liblora/sx1276regs-fsk.h>
#include <liblora/sx1276regs-lora.h>

//#include <libhimax/hm01b0.h>
//
//#include <libjpeg/jpec.h> 
//
//#include "camaroptera-dnn.h"

//#define enable_debug
#define cont_power
//#define print_diff
//#define print_image
//#define print_charging
//#define print_packet
//#define print_jpeg

#ifdef enable_debug
  #include <libio/console.h>
#endif

#define __ro_hifram __attribute__((section(".upper.rodata")))
#define __fram __attribute__((section(".persistent")))

//#define OLD_PINS

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

#define MAC_HDR 													0xDF		
#define DEV_ID														0x09

#define PACKET_SIZE												255
#define HEADER_SIZE												5


//Jpeg quality factor
#define JQ 50

// Image differencing parameters
#define P_THR 40
#define P_DIFF_THR 400

__ro_hifram uint8_t radio_buffer[BUFFER_SIZE];

__ro_hifram uint16_t High_Threshold = 0x0FFA; 	// ~3.004V

extern uint8_t frame[];
extern uint8_t frame_jpeg[];
extern uint16_t fp_track;
extern uint16_t fn_track;

__ro_hifram uint8_t old_frame [19200]= {0};

__ro_hifram uint8_t pixels = 0;

__ro_hifram uint8_t tx_packet_index = 0;
__ro_hifram uint8_t frame_index = 0;
__ro_hifram uint16_t frame_track = 0;
__fram uint8_t camaroptera_state = 0;
__ro_hifram static radio_events_t radio_events;

__ro_hifram int state = 0;
__fram uint8_t predict;
__ro_hifram int i, j;
__ro_hifram uint16_t packet_count, sent_history, last_packet_size; 
__ro_hifram	uint16_t len = 0;
//__ro_hifram jpec_enc_t *e;

__ro_hifram uint8_t index_for_dummy_dnn = 0;
extern uint8_t array_for_dummy_dnn[10];

#ifdef EXPERIMENT_MODE
__ro_hifram uint8_t image_capt_not_sent = 0;
__ro_hifram uint8_t frame_not_empty_status, frame_interesting_status;
#endif

// Different Operating modes ==> Next task ID for tasks {0,1,2,3,4}
// [0] - Capture Image
// [1] - Diff
// [2] - Infer
// [3] - Compress
// [4] - Send Packet
__ro_hifram int8_t camaroptera_mode_1[5] = {3, -1, -1, 4, 0} ; 		// SEND ALL
__ro_hifram int8_t camaroptera_mode_2[5] = {1, 3, -1, 4, 0} ; 		// DIFF + SEND
__ro_hifram int8_t camaroptera_mode_3[5] = {1, 2, 3, 4, 0} ; 			// DIFF + INFER + SEND
//__ro_hifram int8_t camaroptera_mode_3[5] = {1, 0, 0, 0, 0} ; 		// For testing only capture+diff
__ro_hifram int8_t *camaroptera_current_mode = camaroptera_mode_3;
__ro_hifram float threshold_1 = 20.0;
__ro_hifram float threshold_2 = 100.0;
__ro_hifram float charge_rate_sum;
__ro_hifram volatile uint8_t charge_timer_count;
__ro_hifram volatile uint16_t adc_reading;
__ro_hifram volatile uint8_t crash_check_flag;
__fram volatile uint8_t crash_flag;
__ro_hifram uint8_t adc_flag;

void camaroptera_compression();
void camaroptera_init_lora();
void OnTxDone();
float camaroptera_wait_for_charge();
void camaroptera_wait_for_interrupt();
void camaroptera_mode_select(float charge_rate);
uint8_t diff();
uint8_t camaroptera_next_task(uint8_t current_task);
void camaroptera_send_packet();
extern void task_init();

static void init_hw() {
	msp_watchdog_disable();
	msp_gpio_unlock();
	msp_clock_setup();
}

int main(void) {
	init_hw();
	
	while(1){
		P2OUT |= BIT0;
		camaroptera_send_packet();
		__delay_cycles(16000000);
		P2OUT &= ~BIT0;
		}
	return 0;
} // End main()

void camaroptera_send_packet(){

	radio_buffer[0] = MAC_HDR;
	radio_buffer[1] = DEV_ID;
	radio_buffer[2] = frame_index;
	radio_buffer[3] = packet_count;
	radio_buffer[4] = tx_packet_index;
	for( j = HEADER_SIZE; j < PACKET_SIZE; j++ ){
		radio_buffer[j] = (j - HEADER_SIZE) % 10;
		}

	spi_init();

	P4DIR |= BIT4;
	P4OUT |= BIT4;

	__delay_cycles(1600000);
	P2DIR |= BIT1;
	P2OUT |= BIT1;
	camaroptera_init_lora();
	P2OUT &= ~BIT1;
	P2OUT |= BIT1;

	sx1276_send( radio_buffer, PACKET_SIZE );
	
	P2OUT &= ~BIT1;
	P2OUT |= BIT1;
	__bis_SR_register(LPM4_bits+GIE);

	P2OUT &= ~BIT1;
	P2OUT |= BIT1;
	//while(irq_flag != 1);

	sx1276_on_dio0irq();
	P2OUT &= ~BIT1;
	P2OUT |= BIT1;

	P5SEL1 &= ~(BIT0+ BIT1 + BIT2);
	P5SEL0 &= ~(BIT0+ BIT1 + BIT2);
	P5DIR &= ~(BIT0+ BIT1 + BIT2);

	P4OUT &= ~BIT1;
	P4OUT &= ~BIT2;
	P4OUT &= ~BIT4;
	
	P2OUT &= ~BIT1;
	P2DIR &= ~BIT1;
}	


void OnTxDone() {
	tx_packet_index ++;
	frame_track += (PACKET_SIZE - HEADER_SIZE);
}

void camaroptera_init_lora() {
	radio_events.TxDone = OnTxDone;
	//radio_events.RxDone = OnRxDone;
	//radio_events.TxTimeout = OnTxTimeout;
	//radio_events.RxTimeout = OnRxTimeout;
	//radio_events.RxError = OnRxError;

  	sx1276_init(radio_events);
	P2OUT &= ~BIT1;
	P2OUT |= BIT1;
  	sx1276_set_channel(RF_FREQUENCY);
	P2OUT &= ~BIT1;
	P2OUT |= BIT1;

	sx1276_set_txconfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 2000);
}


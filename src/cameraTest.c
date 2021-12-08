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

//#include <liblora/mcu.h>
//#include <liblora/spi.h>
//#include <liblora/sx1276.h>
//#include <liblora/sx1276regs-fsk.h>
//#include <liblora/sx1276regs-lora.h>

#include <libhimax/hm01b0.h>
//
//#include <libjpeg/jpec.h> 
//
//#include "camaroptera-dnn.h"

#define enable_debug
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
#define TX_OUTPUT_POWER										11        // dBm
#define BUFFER_SIZE                       256 // Define the payload size here

#define MAC_HDR 													0xDF		
#define DEV_ID														0x09

#define PACKET_SIZE												1
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

__ro_hifram uint16_t pixels = 0;

__ro_hifram uint8_t tx_packet_index = 0;
__ro_hifram uint8_t frame_index = 0;
__ro_hifram uint16_t frame_track = 0;
__fram uint8_t camaroptera_state = 0;
//__ro_hifram static radio_events_t radio_events;

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
void testCamera();

static void init_hw() {
	msp_watchdog_disable();
	msp_gpio_unlock();
	msp_clock_setup();
}

int main(void) {
	init_hw();
	INIT_CONSOLE();	
	
	PRINTF("Starting\r\n");
	__delay_cycles(16000); 	// POR time for the camera = 50us
		
	while(1){
		//camaroptera_wait_for_charge();
		pixels = 0;
		while(pixels == 0){
			pixels = capture();
			
			}
		PRINTF("Start captured frame\r\n");
		for( i = 0 ; i < pixels ; i++ )
			PRINTF("%u ", frame[i]);
		PRINTF("\r\nEnd frame\r\n");

		__delay_cycles(16000000);
	}

	return 0;
} // End main()

float camaroptera_wait_for_charge(){
#ifdef enable_debug
	//PRINTF("Waiting for cap to be charged. Going To Sleep\n\r");
#endif
	
#ifdef OLD_PINS
    P2SEL0 |= BIT4;                                 //P1.0 ADC mode
    P2SEL1 |= BIT4;                                 //
#else
    //P1SEL0 |= BIT5;                                 //P1.0 ADC mode
    //P1SEL1 |= BIT5;                                 //
    P2SEL0 |= BIT3;                                 //P2.3 ADC mode
    P2SEL1 |= BIT3;                                 //
	P2DIR |= BIT4;
	P2OUT |= BIT4;
#endif
		
	// ======== Configure ADC ========
	// Take single sample when timer triggers and compare with threshold

    ADC12CTL0 &= ~ADC12ENC;  				// Disable conversion before configuring
	ADC12CTL0 |= ADC12SHT0_2 | ADC12ON; 	// Sampling time, S&H=16, ADC12 ON
    ADC12CTL1 |= ADC12SHP | ADC12SHS_0 | ADC12CONSEQ_0 ;      // Use ADC12SC to trigger and single-channel

#ifdef OLD_PINS
    ADC12MCTL0 |= ADC12INCH_7 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
#else
    //ADC12MCTL0 |= ADC12INCH_5 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
    ADC12MCTL0 |= ADC12INCH_6 | ADC12EOS | ADC12WINC;        // A6 ADC input select; Vref+ = AVCC
#endif
	
	/*
   	ADC12IER0 |= ADC12IE0;
	ADC12CTL0 |= (ADC12ENC + ADC12SC);                        // Start sampling/conversion
	
	// Do a single conversion before starting
	__bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts
	ADC12CTL0 &= ~ADC12ENC;
	//PRINTF("Done first adc read\r\n");
	

	if( adc_reading >= High_Threshold ){ 		// Cap fully charged already
		
		ADC12CTL0 &= ~(ADC12ON+ADC12ENC);
		ADC12IER0 &= ~ADC12IE0;
		return 0;
	}
	else{ 										// Cap not fully charged
	*/ //PRINTF("Timing charging\r\n");
	    
   		uint16_t initial_voltage = adc_reading;
		uint16_t voltage_temp;	

		ADC12IER0 &= ~ADC12IE0;
		ADC12HI = High_Threshold;                               // Enable ADC interrupt
    	ADC12IER2 &= ~ADC12HIIE;                                  // Enable ADC threshold interrupt
	
		// ========= Configure Timer =======
		// Timer = 205 = ~50ms
		
		TA0CTL |= TACLR;
		TA0CCR0 = 307; 
		TA0CCTL0 |= CCIE;
		TA0CTL |= TASSEL__ACLK + ID__8 + MC__UP; 	// ACLK = 32768kHz, ID=8 => 1 tick = 244.14us

		charge_timer_count = 0;
		
		adc_flag = 1;
		// Wake from this only on ADC12HIIFG
		__bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts
		
		uint32_t timer_temp = 0;
		timer_temp = (charge_timer_count*TA0CCR0) + TA0R;
		
		TA0CCTL0 &= ~CCIE;
		TA0CTL &= ~TAIE;
		TA0CTL |= TACLR;
		TA0CTL |= MC__STOP;
		ADC12CTL0 &= ~(ADC12ON+ADC12ENC);
		ADC12IER2 &= ~ADC12HIIE;
		ADC12IER0 &= ~ADC12IE0;

#ifdef print_charging
		PRINTF("Timer Value After: (HI)%u", (timer_temp>>16));
		PRINTF("(LO)%u\r\n", (timer_temp & 0xFFFF)) ;
#endif
		
		voltage_temp = adc_reading - initial_voltage;

#ifdef print_charging
		PRINTF("Capacitor Charge Value Changed: %i\r\n", voltage_temp);
#endif
	
		int32_t temp = voltage_temp * 10;
		timer_temp = timer_temp / 1000;
		float charge_rate = temp / timer_temp;
#ifdef print_charging
		PRINTF("\r\nCHARGE RATE: %i\r\n", (int)charge_rate);
#endif

#ifndef OLD_PINS
	P2OUT &= ~BIT4;
#endif
		PRINTF("Done charging\r\n");

		return charge_rate;
	// }
}

// ============= ISR Routines

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void){
	// Triggered every 75ms
	//P7OUT |= BIT2;
	//P7OUT &= ~BIT2;
	
	TA0CCTL0 &= ~CCIFG; 			// Clear Interrupt Flag
	charge_timer_count++; 			// Increment total charging time counter
		
	// If called from cap charging routine
	if(adc_flag){
		ADC12IFGR0 &= ~ADC12IFG0;
		ADC12CTL1 |= ADC12SHP | ADC12SHS_0 | ADC12CONSEQ_0 ;      // Use ADC12SC to trigger and single-channel
		ADC12CTL0 |= (ADC12ON + ADC12ENC + ADC12SC); 			// Trigger ADC conversion
		while(!(ADC12IFGR0 & ADC12IFG0)); 			// Wait till conversion over	
		adc_reading = ADC12MEM0; 					// Read ADC value
		ADC12CTL0 &= ~ADC12ENC; 					// Disable ADC

		PRINTF("%u\r\n", (adc_reading));
		__delay_cycles(1600);

		if(adc_reading >= High_Threshold){ 			// Check if charged
			TA0CCTL0 &= ~CCIE;
			TA0CTL &= ~TAIE;	
			__bic_SR_register_on_exit(LPM3_bits | GIE);
		}
	}	
	else if(crash_check_flag){
		// Triggered every 700ms
		
		//P2OUT |= BIT5;
		//P2OUT &= ~BIT5;
		
		TA0CCTL0 &= ~CCIFG; 			// Clear Interrupt Flag		
		crash_flag = 1;
		TA0CCTL0 &= ~CCIE;
		TA0CTL &= ~TAIE;	
		__bic_SR_register_on_exit(LPM0_bits | GIE);
	}	
}


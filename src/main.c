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

#include <libhimax/hm01b0.h>

#include <libjpeg/jpec.h> 

#include "camaroptera-dnn.h"

#define enable_debug
//#define cont_power
//#define print_image
//#define print_packet
//#define print_jpeg

#ifdef enable_debug
  #include <libio/console.h>
#endif

#define __ro_hifram __attribute__((section(".upper.rodata")))

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

#define High_Threshold 0xFF0         	    // ~2.95V

//Jpeg quality factor
#define JQ 50

// Image differencing parameters
#define P_THR 40
#define P_DIFF_THR 400

__ro_hifram uint8_t radio_buffer[BUFFER_SIZE];

extern uint8_t frame[];
extern uint8_t frame_jpeg[];
__ro_hifram uint8_t old_frame [19200]= {0};

__ro_hifram pixels = 0;

__ro_hifram uint8_t tx_packet_index = 0;
__ro_hifram uint8_t frame_index = 0;
__ro_hifram uint8_t image_capt_not_sent = 0;
__ro_hifram uint16_t frame_track = 0;
__ro_hifram uint8_t camaroptera_state = 0;
__ro_hifram static radio_events_t radio_events;

__ro_hifram int state = 0;
__ro_hifram uint8_t predict;
__ro_hifram int i, j;
__ro_hifram uint16_t packet_count, sent_history, last_packet_size; 
__ro_hifram	uint16_t len = 0;
__ro_hifram jpec_enc_t *e;

__ro_hifram uint8_t index_for_dummy_dnn = 0;
extern uint8_t array_for_dummy_dnn[10];

// Different Operating modes ==> Next task ID for tasks {0,1,2,3,4}
__ro_hifram int8_t camaroptera_mode_1[5] = {3, -1, -1, 4, 0} ; 		// SEND ALL
__ro_hifram int8_t camaroptera_mode_2[5] = {1, 3, -1, 4, 0} ; 		// DIFF + SEND
__ro_hifram int8_t camaroptera_mode_3[5] = {1, 2, 3, 4, 0} ; 			// DIFF + INFER + SEND
//__ro_hifram int8_t camaroptera_mode_3[5] = {3, 0, 0, 0, 0} ; 		// For testing only capture+jpeg
__ro_hifram int8_t *camaroptera_current_mode = camaroptera_mode_1;
__ro_hifram float threshold_1 = 20.0;
__ro_hifram float threshold_2 = 100.0;
__ro_hifram float charge_rate_sum;
__ro_hifram volatile uint8_t charge_timer_count;

void camaroptera_compression();
void camaroptera_init_lora();
void OnTxDone();
float camaroptera_wait_for_charge();
void camaroptera_wait_for_interrupt();
void camaroptera_mode_select(float charge_rate);
uint8_t diff();
uint8_t camaroptera_next_task(uint8_t current_task);
extern void task_init();

int camaroptera_main(void) {
	while(1){

	switch( camaroptera_state ){

		case 0: 									// == CAPTURE IMAGE ==
		
			P8OUT |= BIT3; 
		
#ifndef cont_power
			camaroptera_wait_for_charge(); 			//Wait to charge up 
#endif
			P8OUT ^= BIT1; 

#ifdef enable_debug        	
			PRINTF("Cap ready\n\rSTATE 0: Capturing a photo.\r\n");
#endif

			pixels = capture();

#ifdef print_image
			PRINTF("Captured ---%i--- pixels\r\n", pixels);
			PRINTF("Start captured frame\r\n");
			for( i = 0 ; i < pixels ; i++ )
				PRINTF("%u ", frame[i]);
			PRINTF("\r\nEnd frame\r\n");
#endif
			PRINTF("Done Capturing\r\n");
			camaroptera_state = camaroptera_next_task(0);
			P8OUT ^= BIT1; 
			break;
	
		case 1: 									// == DIFF ==
			
			P8OUT ^= BIT1; 
			
#ifdef enable_debug        	
			PRINTF("STATE 1: Performing Diff.\r\n");
#endif
			// diff();
			if(diff()){
#ifdef enable_debug        	
				PRINTF("Frame is different\r\n");
#endif
				camaroptera_state = camaroptera_next_task(1);
			} else {
#ifdef enable_debug        	
				PRINTF("No change detected\r\n");
#endif
				camaroptera_state = 0;
			}
			
			P8OUT ^= BIT1; 
			
			break;

		case 2: 									// == DNN ==

#ifdef enable_debug        	
			PRINTF("STATE 2: Calling DNN.\r\n");
#endif
#ifndef cont_power
			camaroptera_wait_for_charge(); 			//Wait to charge up 
#endif
			P8OUT ^= BIT1; 

			
			TRANSITION_TO(task_init);
      	
			break;

		case 3: 									// == COMPRESS ==
			
#ifdef enable_debug        	
			PRINTF("STATE 3: Calling JPEG Compression.\r\n");
#endif

#ifndef cont_power
			camaroptera_wait_for_charge(); 			//Wait to charge up 
#endif
			P8OUT ^= BIT1; 

			camaroptera_compression();

#ifdef print_jpeg
			PRINTF("Start JPEG frame\r\n");
			for( i = 0 ; i < len ; i++ )
				PRINTF("%u ", frame_jpeg[i]);
			PRINTF("\r\nEnd JPEG frame\r\n");
#endif
			camaroptera_state = camaroptera_next_task(3);
			
			P8OUT ^= BIT1; 
			
			break;
			
		case 4: 									// == SEND BY RADIO==
			
			P8OUT ^= BIT1; 
			
			charge_rate_sum = 0; 
#ifdef enable_debug        	
			PRINTF("STATE 4: Detected person in Image. Calling Radio.\r\n");
#endif

			packet_count = pixels  / (PACKET_SIZE - HEADER_SIZE);

			last_packet_size = pixels - packet_count * (PACKET_SIZE - HEADER_SIZE) + HEADER_SIZE;

			if(pixels % (PACKET_SIZE - HEADER_SIZE) != 0)
				packet_count ++;

			sent_history = tx_packet_index;

			for( i = sent_history; i < packet_count; i++ ){

				P8OUT ^= BIT2;
				
				radio_buffer[0] = MAC_HDR;
				radio_buffer[1] = DEV_ID;
				radio_buffer[2] = frame_index;
				radio_buffer[3] = packet_count;
				radio_buffer[4] = tx_packet_index;
#ifdef print_packet        	
				PRINTF("START PACKET\r\n");
#endif
				if( i == packet_count - 1){
					for( j = HEADER_SIZE; j < last_packet_size; j++ ){
						radio_buffer[j] = frame_jpeg[frame_track + j - HEADER_SIZE];
#ifdef print_packet       	
						PRINTF("%u ", radio_buffer[j]);
#endif 
						}
					}
				else{
					for( j = HEADER_SIZE; j < PACKET_SIZE; j++ ){
						radio_buffer[j] = frame_jpeg[frame_track + j - HEADER_SIZE];
#ifdef print_packet        	
						PRINTF("%u ", radio_buffer[j]);
#endif 
						}
					}
#ifdef print_packet       	
				PRINTF("\r\nEND PACKET\r\n");
#endif
				
#ifndef cont_power
				//Wait to charge up
				charge_rate_sum += camaroptera_wait_for_charge();
#else
				__delay_cycles(80000000);
#endif

#ifdef enable_debug        	
				PRINTF("Cap ready\n\r");
#endif  

#ifdef OLD_PINS
				P4DIR |= BIT7;
				P4OUT |= BIT7;
#else
				P4DIR |= BIT4;
				P4OUT |= BIT4;
#endif

				spi_init();
				camaroptera_init_lora();

				if( i == packet_count - 1){
#ifdef enable_debug        	
					PRINTF("Sending packet\n\r");
#endif  
					sx1276_send( radio_buffer,  last_packet_size);
					}
				else{
					sx1276_send( radio_buffer, PACKET_SIZE );
					}
				
				__bis_SR_register(LPM4_bits+GIE);

				//while(irq_flag != 1);

				sx1276_on_dio0irq();

#ifdef enable_debug
				PRINTF("Sent packet (ID=%u). Frame at %u. Sent %u till now. %u more to go.\r\n", tx_packet_index, frame_track, tx_packet_index, (packet_count - tx_packet_index));
#endif

				P5SEL1 &= ~(BIT0+ BIT1 + BIT2);
				P5SEL0 &= ~(BIT0+ BIT1 + BIT2);
				P5DIR &= ~(BIT0+ BIT1 + BIT2);

#ifdef OLD_PINS
				P5OUT &= ~BIT3;
				P1OUT &= ~BIT4;
				P4OUT &= ~BIT7;
#else
				P4OUT &= ~BIT1;
				P4OUT &= ~BIT2;
				P4OUT &= ~BIT4;
#endif
				P8OUT ^= BIT2;
				}  // End for i

#ifdef enable_debug
			PRINTF("Sent full image\r\n");
#endif
			tx_packet_index = 0;
			frame_track = 0;
			frame_index++;
			charge_rate_sum = charge_rate_sum / packet_count;
			
			//camaroptera_mode_select( charge_rate_sum );

			camaroptera_state = camaroptera_next_task(4);
			P8OUT ^= BIT1; 
			P8OUT &= ~BIT3; 
			break;
		default:
			camaroptera_state = 0;
			break;
		} // End switch

	} // End while(1)

} // End main()

float camaroptera_wait_for_charge(){

	ADC12IFGR2 &= ~ADC12HIIFG;      // Clear interrupt flag

    P1SEL0 |= BIT5;                                 //P1.0 ADC mode
    P1SEL1 |= BIT5;                                 //

    //Configure ADC
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;                      // Sampling time, S&H=4, ADC12 on
    ADC12CTL1 = ADC12SHP | ADC12SHS_1 | ADC12CONSEQ_2;      // Use TA0.1 to trigger, and repeated-single-channel

#ifdef OLD_PINS
    ADC12MCTL0 = ADC12INCH_7 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
#else
    ADC12MCTL0 = ADC12INCH_5 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
#endif

    ADC12HI = High_Threshold;                               // Enable ADC interrupt
    ADC12IER2 = ADC12HIIE;                                  // Enable ADC threshold interrupt
    ADC12CTL0 |= ADC12ENC | ADC12SC;                        // Start sampling/conversion

    // Configure Timer0_A3 to periodically trigger the ADC12
		TA0CCR0 = 2048-1;                                       // PWM Period
    TA0CCTL1 = OUTMOD_3;                                    // TACCR1 set/reset
    TA0CCR1 = 2047;                                         // TACCR1 PWM Duty Cycle
    TA0CTL = TASSEL__ACLK | MC__UP;                         // ACLK, up mode

		__delay_cycles(10);
		int16_t voltage_temp = ADC12MEM0;
		//PRINTF("Capacitor Charge Value Before: %i\r\n", voltage_temp);
  	
		TA3CTL |= TACLR;
		TA3CTL = TAIE | TASSEL__ACLK | MC__CONTINUOUS;
		charge_timer_count = 0;
		uint32_t timer_temp = 0;
		//PRINTF("Timer Value Before: (HI)%u", timer_temp>>16);
		//PRINTF("(LO)%u\r\n", timer_temp & 0xFFFF);

#ifdef enable_debug
    PRINTF("W8ing for cap to be charged. Going To Sleep\n\r");
#endif

    __bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts

		timer_temp = (charge_timer_count*65536) + TA3R;
		//PRINTF("CHARGE_TIMER_COUNT: %u\r\n", charge_timer_count);		
		PRINTF("Timer Value After: (HI)%u", (timer_temp>>16));
		PRINTF("(LO)%u\r\n", (timer_temp & 0xFFFF)) ;
		//PRINTF("RAW: %u\r\n", TA3R);
		
		voltage_temp = ADC12MEM0 - voltage_temp;
		PRINTF("Capacitor Charge Value Changed: %i\r\n", voltage_temp);
    TA3CTL |= TACLR + MC__STOP;
    TA0CTL |= TACLR + MC__STOP;
    ADC12CTL0 = ~(ADC12ON);

		int32_t temp = voltage_temp * 10;
		timer_temp = timer_temp / 1000;
		float charge_rate = temp / timer_temp;
		PRINTF("\r\nCHARGE RATE: %i\r\n", (int)charge_rate);
		__delay_cycles(10);
		return charge_rate;
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
  sx1276_set_channel(RF_FREQUENCY);

	sx1276_set_txconfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 2000);
}

void camaroptera_compression(){

#ifdef enable_debug        	
	PRINTF("Starting JPEG compression\n\r");
#endif

	e = jpec_enc_new2(frame, 160, 120, JQ);

	jpec_enc_run(e, &len);

	pixels = len;
 
#ifdef enable_debug
	PRINTF("Done. New img size: -- %u -- bytes.\r\n", pixels);
#endif

}

void camaroptera_mode_select( float charge_rate ){
 
#ifdef enable_debug
	PRINTF("Changing Mode to ");
#endif
 	
  if ( charge_rate < threshold_1 ){
    camaroptera_current_mode = camaroptera_mode_3;
#ifdef enable_debug
	PRINTF("Mode 3\r\n");
#endif
	}
  else if ( charge_rate >= threshold_1 && charge_rate < threshold_2 ){
    camaroptera_current_mode = camaroptera_mode_2;
#ifdef enable_debug
	PRINTF("Mode 2\r\n");
#endif
	}
	else if ( charge_rate >= threshold_2 ){
		camaroptera_current_mode = camaroptera_mode_1;
#ifdef enable_debug
	PRINTF("Mode 1\r\n");
#endif
	}
}

uint8_t camaroptera_next_task( uint8_t current_task ){
	
	if ( *(camaroptera_current_mode + current_task) == -1 )
		return 0;
	else
		return *(camaroptera_current_mode + current_task);
}

uint8_t diff(){

	uint16_t i, j = 0;
	for(i = 0; i < pixels; i++){
		if ((frame[i] - old_frame[i] >= P_THR) || (old_frame[i] - frame[i] >= P_THR))
			j++;
	}
														  
	memcpy(old_frame, frame, sizeof(old_frame));
																  
#ifdef enable_debug
	PRINTF("DIFFERENT PIXELS: %u\r\n", j);
#endif


	if (j >= 400)
		return 1;
	else
		return 0;
}

void camaroptera_wait_for_interrupt(){
	
	P8DIR &= ~BIT0; 				// Set as Input
	P8REN |= BIT0; 					// Enable Pullups/downs
	P8OUT &= ~BIT0; 				// Set as Input Pulldown

	P8IES &= ~BIT0; 				// Interrupt on Low->High
	P8IE |= BIT0; 					// Enable Interrupt

	__bis_SR_register( LPM4_bits | GIE );

	}

void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12ISR (void){

    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__ADC12HIIFG:  		    // Vector  6:  ADC12BHI
        ADC12IER2 = ~(ADC12HIIE);
       	__bic_SR_register_on_exit(LPM3_bits | GIE);
        	break;
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0 Interrupt
			//PRINTF("ADC: %i\n\r", ADC12MEM0);
            break;
        default: break;
    }
}

void __attribute__ ((interrupt(TIMER3_A1_VECTOR))) Timer3_A1_ISR (void){
	switch(__even_in_range(TA3IV, TAIV__TAIFG)){
		case TAIV__NONE:   break;           // No interrupt
		case TAIV__TACCR1: break;           // CCR1 not used
		case TAIV__TACCR2: break; 				  // CCR2 not used
		case TAIV__TACCR3: break;           // reserved
		case TAIV__TACCR4: break;           // reserved
		case TAIV__TACCR5: break;           // reserved
		case TAIV__TACCR6: break;           // reserved
		case TAIV__TAIFG:                   // overflow
				charge_timer_count ++;
				TA3CTL &= ~TAIFG;
				break;
		default: 
				break;
	}
}

void __attribute__ ((interrupt(PORT8_VECTOR))) port_8 (void) {
		P8IE &= ~BIT0;
		P8IFG &= ~BIT0;
		__bic_SR_register_on_exit(LPM4_bits+GIE);
}


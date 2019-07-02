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
//#include <liblora/uart.h>
#include <liblora/spi.h>
#include <liblora/sx1276.h>
#include <liblora/sx1276regs-fsk.h>
#include <liblora/sx1276regs-lora.h>

#include <libhimax/hm01b0.h>

#include <libjpeg/jpec.h> 
#include "camaroptera-dnn.h"

#define enable_debug
#define cont_power

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

#define DEV_ID														0x01

#define PACKET_SIZE												255

#define High_Threshold 0xFF0         	    // ~2.95V

//Jpeg quality factor
#define JQ 50

__ro_hifram uint8_t buffer[BUFFER_SIZE];

extern uint8_t frame[];
extern uint8_t frame_jpeg[];

__ro_hifram pixels = 0;

__ro_hifram uint8_t tx_packet_index = 0;
__ro_hifram uint16_t sent_packet_count = 0;
__ro_hifram uint8_t image_capt_not_sent = 0;
__ro_hifram uint16_t frame_track = 0;
__ro_hifram uint8_t camaroptera_state = 0;
__ro_hifram static radio_events_t radio_events;

__ro_hifram int state = 0;
__ro_hifram int i, j;
__ro_hifram uint16_t packet_count, sent_history, last_packet_size; 
__ro_hifram	uint16_t len = 0;
__ro_hifram jpec_enc_t *e;

void camaroptera_compression();
void camaroptera_init_lora();
void OnTxDone();
void camaroptera_wait_for_charge();
uint8_t camaroptera_next_task(uint8_t current_task);
extern void task_init();

int camaroptera_main(void) {
	
	while(1){

	switch( camaroptera_state ){

		case 0: 									// == CAPTURE IMAGE ==
			
#ifndef cont_power
			camaroptera_wait_for_charge(); 			//Wait to charge up 
#endif

#ifdef enable_debug        	
			PRINTF("Cap ready\n\rSTATE 0: Capturing a photo.\r\n");
#endif

			P8OUT |= BIT2;
			pixels = capture();
			P8OUT &= ~BIT2;

#ifdef print_image
			PRINTF("Start captured frame\r\n");
			for( i = 0 ; i < pixels ; i++ )
				PRINTF("%u ", frame[i]);
			PRINTF("\r\nEnd frame\r\n");
#endif
			camaroptera_state = camaroptera_next_task(0);
			break;
	
		case 1: 									// == DIFF ==
			
#ifdef enable_debug        	
			PRINTF("STATE 1: Performing Diff.\r\n");
#endif
			// diff();
			camaroptera_state = camaroptera_next_task(1);
			break;

		case 2: 									// == DNN ==
			
#ifdef enable_debug        	
			PRINTF("STATE 2: Calling DNN.\r\n");
#endif
			// dnn();
			TRANSITION_TO(task_init);
			break;

		case 3: 									// == COMPRESS ==

#ifdef enable_debug        	
			PRINTF("STATE 3: Calling JPEG Compression.\r\n");
#endif
			P8OUT |= BIT2;
			camaroptera_compression();
			P8OUT &= ~BIT2;
#ifdef print_image
			PRINTF("Start JPEG frame\r\n");
			for( i = 0 ; i < len ; i++ )
				PRINTF("%u ", frame_jpeg[i]);
			PRINTF("\r\nEnd JPEG frame\r\n");
#endif
			camaroptera_state = camaroptera_next_task(3);
			break;
			
		case 4: 									// == SEND BY RADIO==

#ifdef enable_debug        	
			PRINTF("STATE 4: Calling Radio.\r\n");
#endif

			packet_count = pixels  / (PACKET_SIZE - 2);

			last_packet_size = pixels - packet_count * (PACKET_SIZE - 2) + 2;

			if(pixels % (PACKET_SIZE - 2) != 0)
				packet_count ++;

			sent_history = sent_packet_count;

			for( i = sent_history; i < packet_count; i++ ){

				P8OUT |= BIT1; 
				buffer[0] = DEV_ID;
				buffer[1] = tx_packet_index;
#ifdef enable_debug        	
	      PRINTF("START PACKET\r\n");
#endif
				if( i == packet_count - 1){
					for( j = 2; j < last_packet_size; j++ ){
						buffer[j] = frame_jpeg[frame_track + j - 2];
#ifdef enable_debug        	
       			PRINTF("%u ", buffer[j]);
#endif 
						}
					}
				else{
					for( j = 2; j < PACKET_SIZE; j++ ){
						buffer[j] = frame_jpeg[frame_track + j - 2];
#ifdef enable_debug        	
    		    PRINTF("%u ", buffer[j]);
#endif 
						}
					}
#ifdef enable_debug        	
        PRINTF("\r\nEND PACKET\r\n");
#endif
				
#ifndef cont_power
				//Wait to charge up
				camaroptera_wait_for_charge();
#else
				__delay_cycles(16000000);
#endif

#ifdef enable_debug        	
        PRINTF("Cap ready\n\r");
#endif  

#ifdef OLD_PINS
				P4OUT |= BIT7;
#else
				P4OUT |= BIT4;
#endif

				spi_init();

				P8OUT |= BIT2;
				camaroptera_init_lora();
				P8OUT &= ~BIT2;


				if( i == packet_count - 1){
#ifdef enable_debug        	
					PRINTF("Sending packet\n\r");
#endif  
					P8OUT |= BIT2;
					sx1276_send( buffer,  last_packet_size);
					P8OUT &= ~BIT2;
					}
				else{
					P8OUT |= BIT2;
					sx1276_send( buffer, PACKET_SIZE );
					P8OUT &= ~BIT2;
					}

				__bis_SR_register(LPM4_bits+GIE);

				while(irq_flag != 1);

				P8OUT |= BIT2;
				sx1276_on_dio0irq();
				P8OUT &= ~BIT2;

				irq_flag = 0;

#ifdef enable_debug
				PRINTF("Sent packet (ID=%u). Frame at %u. Sent %u till now. %u more to go.\r\n", tx_packet_index, frame_track, sent_packet_count, (packet_count - sent_packet_count));
#endif

				P8OUT &= ~BIT1;
				P5SEL1 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
				P5SEL0 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
				P5DIR &= ~(BIT0+ BIT1 + BIT2 + BIT3);

#ifdef OLD_PINS
				P4OUT &= ~BIT7;
#else
				P4OUT &= ~BIT4;
#endif
				}  // End for i

#ifdef enable_debug
				PRINTF("Sent full image\r\n");
#endif
				tx_packet_index = 0;
				sent_packet_count = 0;
				frame_track = 0;

				camaroptera_state = camaroptera_next_task(4);
				break;

		default:
				camaroptera_state = 0;
				break;
		} // End switch

	} // End while(1)

} // End main()

void camaroptera_wait_for_charge(){

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

#ifdef enable_debug
    PRINTF("W8ing for cap to be charged. Going To Sleep\n\r");
#endif

    __bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts

    TA0CTL |= TACLR + MC__STOP;
    ADC12CTL0 = ~(ADC12ON);
}

void OnTxDone() {
	P8OUT &= ~BIT2;
	tx_packet_index ++;
	sent_packet_count ++;
	frame_track += 253;
}

void camaroptera_init_lora() {
  radio_events.TxDone = OnTxDone;

  sx1276_init(radio_events);
  sx1276_set_channel(RF_FREQUENCY);

  sx1276_set_txconfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 2000);
}

void camaroptera_compression(){

	P8OUT |= BIT3;


#ifdef enable_debug        	
	PRINTF("\n\rStarting JPEG compression\n\r");
#endif

	e = jpec_enc_new2(frame, 160, 120, JQ);

	jpec_enc_run(e, &len);

	pixels = len;

#ifdef enable_debug
	PRINTF("Done. New img size: -- %u -- bytes.\r\n", pixels);
#endif

	P8OUT &= ~BIT3;
}

uint8_t camaroptera_next_task( uint8_t current_task ){
	return current_task + 1;
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

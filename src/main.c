#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>
#include <libmsp/mem.h>

#include <liblora/mcu.h>
//#include <liblora/uart.h>
#include <liblora/spi.h>
#include <liblora/sx1276.h>
#include <liblora/sx1276regs-fsk.h>
#include <liblora/sx1276regs-lora.h>

#include <libhimax/hm01b0.h>

#include "jpec.h" 

#define enable_debug

#ifdef enable_debug
  #include <libio/console.h>
#endif


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

//Buffer offsett for jpec
#define offset 19200

//Jpeg quality factor
#define JQ 50

uint8_t buffer[BUFFER_SIZE];

extern uint8_t frame[];

__nv uint8_t tx_packet_index = 0;
__nv uint16_t sent_packet_count = 0;
__nv uint8_t image_capt_not_sent = 0;
__nv uint16_t frame_track = 0;

__nv HM01B0 cam = {0};

static radio_events_t radio_events;

int state = 0;

void process();
void rf_init_lora();
void OnRxError();
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxDone();
void SendPing();
void wait_for_charge();
void capture();

int main(void) {
	
	int i, j;
	uint16_t packet_count, sent_history, last_packet_size; 

	PM5CTL0 &= ~LOCKLPM5;

	
	P8OUT |= BIT1;					// To demarcate start and end of individual runs of the program
	P8OUT &= ~BIT2; 	  		// To demarcate smaller sections of the program
	P8OUT &= ~BIT3;
	P8DIR |= BIT1 + BIT2 + BIT3;

	mcu_init();

	P6OUT &= ~BIT1;			// Power to Camera
	P6DIR |= BIT1;

	P4OUT &= ~BIT7;			// Power to Radio
	P4DIR |= BIT7;

#ifdef enable_debug
		INIT_CONSOLE();
		PRINTF("Starting the transmitter.\r\n");
#endif
	while(1){

	//================== Camera Code begins here ==================


	if(image_capt_not_sent == 0){

		//Wait to charge up
		wait_for_charge();

#ifdef enable_debug        	
        	PRINTF("Cap ready\n\rCapturing a photo.\r\n");
#endif

			P6OUT |= BIT1;
			P8OUT |= BIT2;
			capture();
			P8OUT &= ~BIT2;
			P6OUT &= ~BIT1;

#ifdef enable_debug
			PRINTF("\r\nStart frame\r\n");

			for( i = 0 ; i < cam.pixels ; i++ ){

				PRINTF("%u ", frame[i]);

			}

			PRINTF("\r\nEnd frame\r\n");
#endif
			P8OUT |= BIT2;
			process();
			P8OUT &= ~BIT2;

			image_capt_not_sent = 1;

#ifdef enable_debug
			PRINTF("\r\nStart frame\r\n");

			for( i = 0 ; i < cam.pixels ; i++ ){

				PRINTF("%u ", frame[offset + i]);

			}

			PRINTF("\r\nEnd frame\r\n");
#endif

		//Wait to charge up
		wait_for_charge();

#ifdef enable_debug        	
        	PRINTF("Cap ready\n\r");
#endif  
	}
	else{

#ifdef enable_debug
			PRINTF("Already have a stored photo.\r\n");
#endif

	}

#ifdef enable_debug
	PRINTF("It's a photo of -- %n -- bytes.\r\n", cam.pixels);
#endif

	//================== Camera Code ends here ==================


	//================== Radio Transmission begins here ==================

	packet_count = cam.pixels  / (PACKET_SIZE - 2);

	last_packet_size = cam.pixels - packet_count * (PACKET_SIZE - 2) + 2;

	if(cam.pixels % (PACKET_SIZE - 2) != 0){

		packet_count ++;

	}

	sent_history = sent_packet_count;

	for( i = sent_history; i < packet_count; i++ ){

		P8OUT |= BIT1; 

		buffer[0] = DEV_ID;
		buffer[1] = tx_packet_index;

		if( i == packet_count - 1){

			for( j = 2; j < last_packet_size; j++ ){

				buffer[j] = frame[offset + frame_track + j - 2];
			}
		}
		else{
			for( j = 2; j < PACKET_SIZE; j++ ){

				buffer[j] = frame[offset + frame_track + j - 2];
			}
		}

		//Wait to charge up
		wait_for_charge();

#ifdef enable_debug        	
        PRINTF("Cap ready\n\r");
#endif  

		P4OUT |= BIT7;
		spi_init();

		P8OUT |= BIT2;
		rf_init_lora();
		P8OUT &= ~BIT2;


		if( i == packet_count - 1){

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
		P4OUT &= ~BIT7;

	}

#ifdef enable_debug
	PRINTF("Sent full image\r\n");
#endif

	tx_packet_index = 0;
	sent_packet_count = 0;
	frame_track = 0;
	image_capt_not_sent = 0;

	//================== Radio Transmission ends here ==================
	
	}

}
void capture(){

	uint16_t id = 0;

	P8OUT |= BIT3;

	hm01b0_init();

	id = hm01b0_reg_default_init();

	P8OUT &= ~BIT3;

#ifdef enable_debug
	PRINTF("Camera ID 0x%04x\n\r", id);	
#endif

	cam.mode = Streaming3;		// External trigger
	cam.state = sleep;			// Camera inited but to be configured
	cam.resolution = QQVGA;		// QQVGA resolution
	cam.dataDepth = EightB;		// 8-bit per pixel
	cam.dataIo = EightL;		// 8 Data lines
	
	P8OUT |= BIT3;
	hm01b0_capture(&cam);

	hm01b0_deinit();
	P8OUT &= ~BIT3;
}

void wait_for_charge(){

	ADC12IFGR2 &= ~ADC12HIIFG;      // Clear interrupt flag

    P2SEL0 |= BIT4;                                 //P1.0 ADC mode
    P2SEL1 |= BIT4;                                 //

    //Configure ADC
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;                      // Sampling time, S&H=4, ADC12 on
    ADC12CTL1 = ADC12SHP | ADC12SHS_1 | ADC12CONSEQ_2;      // Use TA0.1 to trigger, and repeated-single-channel
    ADC12MCTL0 = ADC12INCH_7 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
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

void SendPing() {
   sx1276_send(buffer, 5);
}

void OnTxDone() {
 // uart_write("$TXS\n");
  //if(state == 1) sx1276_set_rx(0);
	P8OUT &= ~BIT2;
	tx_packet_index ++;
	sent_packet_count ++;
	if(sent_packet_count < 19)
		frame_track += 253;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {

	PRINTF("Packet Received.\r\n");
	PRINTF("RSSI: %d dBm\r\n", rssi);
	PRINTF("SNR: %d dB\r\n", snr);
	PRINTF("Received -- %d -- bytes.\r\n", size);
	PRINTF("Packet Contents: %s.\r\n", (char *)payload);

}

void OnRxError() {
	PRINTF("RX Error Detected.\r\n");
}

void rf_init_lora() {
  radio_events.TxDone = OnTxDone;
  radio_events.RxDone = OnRxDone;
  //radio_events.TxTimeout = OnTxTimeout;
  //radio_events.RxTimeout = OnRxTimeout;
  radio_events.RxError = OnRxError;

  sx1276_init(radio_events);
  sx1276_set_channel(RF_FREQUENCY);

  sx1276_set_txconfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 2000);

  sx1276_set_rxconfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

}

void process(){

	P8OUT |= BIT3;

	uint16_t len = 0;

#ifdef enable_debug        	
	PRINTF("\n\rStarting JPEG compression\n\r");
#endif

	jpec_enc_t *e = jpec_enc_new2(frame, cam.W, cam.H, JQ);

	jpec_enc_run(e, &len);

#ifdef enable_debug
	PRINTF("Done. New img size: -- %u -- bytes.\r\n", len);
#endif

	cam.pixels = len;

	P8OUT &= ~BIT3;
}

void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12ISR (void){

    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__NONE:        break;   // Vector  0:  No interrupt
        case ADC12IV__ADC12OVIFG:  break;   // Vector  2:  ADC12MEMx Overflow
        case ADC12IV__ADC12TOVIFG: break;   // Vector  4:  Conversion time overflow
        case ADC12IV__ADC12HIIFG:  		    // Vector  6:  ADC12BHI
        ADC12IER2 = ~(ADC12HIIE);
       	__bic_SR_register_on_exit(LPM3_bits | GIE);
        	break;
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0 Interrupt
			//PRINTF("ADC: %i\n\r", ADC12MEM0);
            break;
        case ADC12IV__ADC12IFG1:   break;   // Vector 14:  ADC12MEM1
        case ADC12IV__ADC12IFG2:   break;   // Vector 16:  ADC12MEM2
        case ADC12IV__ADC12IFG3:   break;   // Vector 18:  ADC12MEM3
        case ADC12IV__ADC12IFG4:   break;   // Vector 20:  ADC12MEM4
        case ADC12IV__ADC12IFG5:   break;   // Vector 22:  ADC12MEM5
        case ADC12IV__ADC12IFG6:   break;   // Vector 24:  ADC12MEM6
        case ADC12IV__ADC12IFG7:   break;   // Vector 26:  ADC12MEM7
        case ADC12IV__ADC12IFG8:   break;   // Vector 28:  ADC12MEM8
        case ADC12IV__ADC12IFG9:   break;   // Vector 30:  ADC12MEM9
        case ADC12IV__ADC12IFG10:  break;   // Vector 32:  ADC12MEM10
        case ADC12IV__ADC12IFG11:  break;   // Vector 34:  ADC12MEM11
        case ADC12IV__ADC12IFG12:  break;   // Vector 36:  ADC12MEM12
        case ADC12IV__ADC12IFG13:  break;   // Vector 38:  ADC12MEM13
        case ADC12IV__ADC12IFG14:  break;   // Vector 40:  ADC12MEM14
        case ADC12IV__ADC12IFG15:  break;   // Vector 42:  ADC12MEM15
        case ADC12IV__ADC12IFG16:  break;   // Vector 44:  ADC12MEM16
        case ADC12IV__ADC12IFG17:  break;   // Vector 46:  ADC12MEM17
        case ADC12IV__ADC12IFG18:  break;   // Vector 48:  ADC12MEM18
        case ADC12IV__ADC12IFG19:  break;   // Vector 50:  ADC12MEM19
        case ADC12IV__ADC12IFG20:  break;   // Vector 52:  ADC12MEM20
        case ADC12IV__ADC12IFG21:  break;   // Vector 54:  ADC12MEM21
        case ADC12IV__ADC12IFG22:  break;   // Vector 56:  ADC12MEM22
        case ADC12IV__ADC12IFG23:  break;   // Vector 58:  ADC12MEM23
        case ADC12IV__ADC12IFG24:  break;   // Vector 60:  ADC12MEM24
        case ADC12IV__ADC12IFG25:  break;   // Vector 62:  ADC12MEM25
        case ADC12IV__ADC12IFG26:  break;   // Vector 64:  ADC12MEM26
        case ADC12IV__ADC12IFG27:  break;   // Vector 66:  ADC12MEM27
        case ADC12IV__ADC12IFG28:  break;   // Vector 68:  ADC12MEM28
        case ADC12IV__ADC12IFG29:  break;   // Vector 70:  ADC12MEM29
        case ADC12IV__ADC12IFG30:  break;   // Vector 72:  ADC12MEM30
        case ADC12IV__ADC12IFG31:  break;   // Vector 74:  ADC12MEM31
        case ADC12IV__ADC12RDYIFG: break;   // Vector 76:  ADC12RDY
        default: break;
    }
}

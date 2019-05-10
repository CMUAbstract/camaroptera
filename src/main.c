#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <liblora/mcu.h>
#include <liblora/uart.h>
#include <liblora/spi.h>
#include <liblora/sx1276.h>
#include <liblora/sx1276regs-fsk.h>
#include <liblora/sx1276regs-lora.h>
#include <libmsp/mem.h>

#include <libov7670/ov7670.h>

#include <libio/console.h>

#include "jpec.h" 

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

#define High_Threshold 0xF62         // ~3.15V

#define enable_debug

void wait_for_charge();

uint8_t buffer[BUFFER_SIZE];
char temp[30];

extern uint8_t frame[];

__nv uint8_t tx_packet_index = 0;
__nv uint16_t sent_packet_count = 0;
__nv uint16_t nb = 0;
__nv uint8_t image_capt_not_sent = 0;
__nv uint16_t frame_track = 0;

static radio_events_t radio_events;

int state = 0;

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
  uart_write("Packet Received.\r\n");

	uart_write("RSSI: ");
	sprintf( temp, "%d", rssi);
	uart_write(temp);
  uart_write("dBm\r\n");
	
	uart_write("SNR: ");
	sprintf( temp, "%d", snr);
	uart_write(temp);
  uart_write("dB\r\n");

	uart_write("Received -- ");
	sprintf( temp, "%d", size);
	uart_write(temp);
  uart_write(" -- bytes.\r\n");

	uart_write("Packet Contents: ");
	uart_write((char *)payload);
  uart_write(" .\r\n");

  //if(state == 1) SendPing();
}

void OnRxError() {
  uart_write("RX Error Detected.\r\n");
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
	
	uint16_t i, j;
	uint16_t total;
	uint16_t average;

	for( i = 0; i < nb; i+=2 ){
		frame[i/2] = frame[i];
		}

	nb /= 2;

	int len = 0;
#ifdef enable_debug        	
	uart_write("Starting JPEG compression\n\r");
#endif

	jpec_enc_t *e = jpec_enc_new2(frame, 160, 120, 50);

	uint8_t *jpeg = jpec_enc_run(e, &len);

	for( i = 0; i < len; i++ ){

		frame[i] = *jpeg;
		jpeg++;

	}

#ifdef enable_debug
	sprintf(temp, "Done. New img size:  %u bytes.\r\n", len);
	uart_write(temp);
#endif

	nb = len;

	// for( i = 0; i < 120; i+=2 ){
	// 	for( j = 0; j < 160; j+=2 ){
	// 			total = frame[i*160+j] +frame[i*160+(j+1)] + frame[(i+1)*160+j] +	frame[(i+1)*160+(j+1)];
	// 			average = total/4;
	// 			frame[((i*160)/4) + j/2] = (uint8_t)average;
	// 		}
	// 	}

	// nb /= 4;

}

int main(void) {
	
	int i, j;
	uint16_t packet_count, sent_history, last_packet_size; 
	char temp[100];

	PM5CTL0 &= ~LOCKLPM5;

	P8DIR |= BIT1 + BIT2;
	P8OUT |= BIT1;			// To demarcate start and end of individual runs of the program
	P8OUT &= ~BIT2; 	  	// To demarcate smaller sections of the program

	mcu_init();

	P6OUT &= ~BIT1;			// Power to Camera
	P6DIR |= BIT1;

	P4OUT &= ~BIT7;			// Power to Radio
	P4DIR |= BIT7;

#ifdef enable_debug
		uart_init();
		uart_write("Starting the transmitter.\r\n");
#endif
	while(1){

	//================== Camera Code begins here ==================


	if(image_capt_not_sent == 0){

		//Wait to charge up
		wait_for_charge();

#ifdef enable_debug        	
        	uart_write("Cap ready\n\r");
			uart_write("Capturing a photo.\r\n");
#endif

			P6OUT |= BIT1;
			P8OUT |= BIT2;
			nb = ov7670_get_photo();
			P8OUT &= ~BIT2;
			P6OUT &= ~BIT1;

#ifdef enable_debug
			uart_write("\r\nStart frame\r\n");
			for( i = 0 ; i < nb ; i++ ){
				uart_printhex8(frame[i]);
			}
			uart_write("\r\nEnd frame\r\n");
#endif

			process();
			image_capt_not_sent = 1;

#ifdef enable_debug
			uart_write("\r\nStart frame\r\n");
			for( i = 0 ; i < nb ; i++ ){
				uart_printhex8(frame[i]);
			}
			uart_write("\r\nEnd frame\r\n");
#endif

		//Wait to charge up
		wait_for_charge();

#ifdef enable_debug        	
        	uart_write("Cap ready\n\r");
#endif  
	}
	else{
#ifdef enable_debug
			uart_write("Already have a stored photo.\r\n");
#endif
	}

#ifdef enable_debug
	sprintf(temp, "It's a photo of %u bytes.\r\n", nb);
	uart_write(temp);
#endif

	//================== Camera Code ends here ==================


	//================== Radio Transmission begins here ==================

	packet_count = nb/(PACKET_SIZE-2);

	last_packet_size = nb - packet_count*(PACKET_SIZE - 2) + 2;

	if(nb % (PACKET_SIZE - 2) != 0){
		packet_count ++;
	}


	sent_history = sent_packet_count;

	for( i = sent_history; i < packet_count; i++ ){
		P8OUT |= BIT1; 

		buffer[0] = DEV_ID;
		buffer[1] = tx_packet_index;

		if( i == packet_count - 1){
			for( j = 2; j < last_packet_size; j++ ){
				buffer[j] = frame[frame_track + j - 2];
			}
		}
		else{
			for( j = 2; j < PACKET_SIZE; j++ ){
				buffer[j] = frame[frame_track + j - 2];
			}
		}

		//Wait to charge up
		wait_for_charge();

#ifdef enable_debug        	
        	uart_write("Cap ready\n\r");
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
		sprintf(temp, "Sent packet (ID=%d). Frame at %d. Sent %d till now. %d more to go.\r\n",	tx_packet_index, frame_track, sent_packet_count, packet_count	- sent_packet_count);
		uart_write(temp);
#endif

		P8OUT &= ~BIT1;
		P5SEL1 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P5SEL0 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P5DIR &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P4OUT &= ~BIT7;

	}

#ifdef enable_debug
	uart_write("Sent full image\r\n");
#endif

	tx_packet_index = 0;
	sent_packet_count = 0;
	frame_track = 0;
	image_capt_not_sent = 0;
	nb = 0;

	//================== Radio Transmission ends here ==================
	
	}

}

void wait_for_charge(){

	ADC12IFGR2 &= ~ADC12HIIFG;      // Clear interrupt flag

    P1SEL0 |= BIT0;                                 //P1.0 ADC mode
    P1SEL1 |= BIT0;                                 //

    //Configure ADC
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;                      // Sampling time, S&H=4, ADC12 on
    ADC12CTL1 = ADC12SHP | ADC12SHS_1 | ADC12CONSEQ_2;      // Use TA0.1 to trigger, and repeated-single-channel
    ADC12MCTL0 = ADC12INCH_0 | ADC12EOS | ADC12WINC;        // A0 ADC input select; Vref+ = AVCC
    ADC12HI = High_Threshold;                               // Enable ADC interrupt
    ADC12IER2 = ADC12HIIE | ADC12INIE;                      // Enable ADC threshold interrupt
    ADC12CTL0 |= ADC12ENC | ADC12SC;                        // Start sampling/conversion

    // Configure Timer0_A3 to periodically trigger the ADC12
    TA0CCR0 = 2048-1;                                       // PWM Period
    TA0CCTL1 = OUTMOD_3;                                    // TACCR1 set/reset
    TA0CCR1 = 2047;                                         // TACCR1 PWM Duty Cycle
    TA0CTL = TASSEL__ACLK | MC__UP;                         // ACLK, up mode
#ifdef enable_debug
    uart_write("W8ing for cap to be charged. Going To Sleep\n\r");
#endif

    __bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts

    TA0CTL |= TACLR + MC__STOP;
    ADC12CTL0 = ~(ADC12ON);
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

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
#define TX_OUTPUT_POWER                   7        // dBm
#define BUFFER_SIZE                       256 // Define the payload size here

uint8_t buffer[BUFFER_SIZE];
char temp[30];

extern uint8_t frame[];

__nv uint8_t packet_track = 0;
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
	packet_track ++;
	frame_track += 20;
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

int main(void) {
 
	PM5CTL0 &= ~LOCKLPM5;

	P8DIR |= BIT1 + BIT2;
	P8OUT |= BIT1;			// To demarcate start and end of individual runs of the program
	P8OUT &= ~BIT1; 	  // To demarcate smaller sections of the program

	mcu_init();

	P6OUT &= ~BIT1;			// Power to Camera
	P6DIR |= BIT1;

	P4OUT &= ~BIT7;			// Power to Radio
	P4DIR |= BIT7;

  //uart_init();
  
	//uart_write("Starting the transmitter.\r\n");

	//================== Camera Code begins here ==================

	P6OUT |= BIT1;

	uint16_t nb = ov7670_get_photo();

	P6OUT &= ~BIT1;

	//================== Camera Code ends here ==================

	//================== Radio Transmission begins here ==================

	int i;
	uint16_t packet_count; 

	packet_count = nb/20;
	
	if(nb % 20 != 0)
		packet_count ++;

	buffer[0] = packet_track;

	for( i = 1; i < 21; i++ ){
			buffer[i] = frame[frame_track + i - 1];
		}

	for( i = 0; i < 1; i++ ){
		P8OUT |= BIT1; 

		TA0CCTL0 = CCIE;
		TA0CCR0 = 50000;
		TA0CTL = TASSEL__ACLK | MC__UP | ID__2;
	
		TA0CTL |= TAIE;
	
		__bis_SR_register(LPM3_bits+GIE);

		P4OUT |= BIT7;
		spi_init();

		P8OUT |= BIT2;
		rf_init_lora();
		P8OUT &= ~BIT2;
		
		P8OUT |= BIT2;
		sx1276_send( buffer, 21 );
		P8OUT &= ~BIT2;

		__bis_SR_register(LPM4_bits+GIE);

		while(irq_flag != 1);

		P8OUT |= BIT2;
		sx1276_on_dio0irq();
		P8OUT &= ~BIT2;

		irq_flag = 0;
	
		P8OUT &= ~BIT1;
		P5SEL1 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P5SEL0 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P5DIR &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P4OUT &= ~BIT7;

	}
	
	//================== Radio Transmission ends here ==================
	
	__bis_SR_register(LPM4_bits);


}

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_isr (void) {
	TA0CCTL0 &= ~CCIE;
	TA0CTL &= ~TAIE;
	TA0CTL &= ~TAIFG;
	TA0CTL |= TACLR + MC__STOP;
	__bic_SR_register_on_exit(LPM3_bits+GIE);

}


#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <liblora/mcu.h>
#include <liblora/uart.h>
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
#define TX_OUTPUT_POWER                   7        // dBm
#define BUFFER_SIZE                       256 // Define the payload size here

uint8_t buffer[BUFFER_SIZE];
char temp[30];

static radio_events_t radio_events;

int state = 0;

void SendPing() {
   sx1276_send(buffer, 5);
}

void OnTxDone() {
	//uart_write("Packet Sent.\r\n");
  //if(state == 1) sx1276_set_rx(0);
	P4OUT &= ~BIT1;
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
  uart_write("$RXE\n");
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
                                  true, 0, 4, LORA_IQ_INVERSION_ON, 2000);

  sx1276_set_rxconfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

}

int main(void) {
 
	PM5CTL0 &= ~LOCKLPM5;

	// Indicate start of program
	P8DIR |= BIT1;
	P8OUT |= BIT1; 

	mcu_init();

	P3SEL0 &= ~BIT4;
	P3SEL1 |= BIT4;
	P3DIR |= BIT4;

	P4OUT &= ~BIT1;
	P4DIR |= BIT1; 	 // To demarcate sections of the program

	P6OUT &= ~BIT1;
	P6DIR |= BIT1;

  //uart_init();
  
	//uart_write("Starting the transmitter.\r\n");

	int i;
	
	buffer[0] = 'H';
	buffer[1] = 'e';
	buffer[2] = 'l';
	buffer[3] = 'l';
	buffer[4] = 'o';
	//buffer[5] = '0';

	for( i = 5; i < BUFFER_SIZE; i++ ){
		buffer[i] = 48 + (i-5)%10;
		}

	__delay_cycles(8000);

	for( i = 0; i < 1; i++ ){
		P8OUT |= BIT1; 

		TA0CCTL0 = CCIE;
		TA0CCR0 = 50000;
		TA0CTL = TASSEL__ACLK | MC__UP | ID__2;
	
		//P8OUT ^= BIT1;
		TA0CTL |= TAIE;
	
		__bis_SR_register(LPM3_bits+GIE);

		//P8OUT ^= BIT1;

		P6OUT |= BIT1;
		spi_init();

		P4OUT |= BIT1;
		rf_init_lora();
		P4OUT &= ~BIT1;
		
		P4OUT |= BIT1;
		sx1276_send( buffer, 1 );
		P4OUT &= ~BIT1;

		__bis_SR_register(LPM4_bits+GIE);

		while(irq_flag != 1);

		P4OUT |= BIT1;
		sx1276_on_dio0irq();
		P4OUT &= ~BIT1;

		irq_flag = 0;
		//uint8_t paConfig, paDac;
		//paConfig = sx1276_read(REG_PACONFIG);
		//paDac =    sx1276_read(REG_PADAC);
		//uart_printhex32(paConfig);
		//uart_printhex32(paDac);
	
		//__delay_cycles(4000000); 	// 500ms
		P8OUT &= ~BIT1;
		P5SEL1 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P5SEL0 &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P5DIR &= ~(BIT0+ BIT1 + BIT2 + BIT3);
		P6OUT &= ~BIT1;
		//__bis_SR_register(LPM4_bits);

	}


}

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_isr (void) {
	TA0CCTL0 &= ~CCIE;
	TA0CTL &= ~TAIE;
	TA0CTL &= ~TAIFG;
	TA0CTL |= TACLR + MC__STOP;
//	P4OUT |= BIT1;
	__bic_SR_register_on_exit(LPM3_bits+GIE);

}


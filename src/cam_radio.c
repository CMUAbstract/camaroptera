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

#include "cam_util.h"
#include "cam_radio.h"
#include "cam_lora.h"
#include "cam_framebuffer.h"

extern uint8_t camaroptera_state;
extern uint8_t *cam_downsamp_buf;
__ro_hifram float charge_rate_sum;

__ro_hifram uint8_t radio_buffer[BUFFER_SIZE];
__ro_hifram uint8_t tx_packet_index = 0;
__ro_hifram static radio_events_t radio_events;
__ro_hifram uint16_t packet_count, sent_history, last_packet_size; 

__ro_hifram uint8_t frame_index = 0;
__ro_hifram uint16_t frame_track = 0;
__ro_hifram int tx_i, tx_j; /*TODO: Why are loop iterators in FRAM?*/

void camaroptera_transmit(size_t num_pixels){
#ifdef EXPERIMENT_MODE
  if(frame_interesting_status){
    P2OUT |= BIT3;
  }
  num_pixels = 1800;     //TODO: BML: Kill constant
  P6OUT |= BIT7;     // Running: Transmission
  P5OUT |= BIT5;     // Signal start
#endif // EXPERIMENT_MODE      

  charge_rate_sum = 0; 

  uint8_t * frame_data = NULL;
#ifndef cam_downsamp
  frame_data = frame_jpeg;
#else
  frame_data = cam_downsamp_buf; 
#endif


#ifdef enable_debug    
  PRINTF("STATE 4: Camaroptera transmit start.\r\n");
#endif

  packet_count = num_pixels  / (PACKET_SIZE - HEADER_SIZE);

  last_packet_size = num_pixels - packet_count * (PACKET_SIZE - HEADER_SIZE) + HEADER_SIZE;

  if(num_pixels % (PACKET_SIZE - HEADER_SIZE) != 0){
    packet_count ++;
  }

  sent_history = tx_packet_index;

  for( tx_i = sent_history; tx_i < packet_count; tx_i++ ){

    P8OUT ^= BIT2;
  
    radio_buffer[0] = MAC_HDR;
    radio_buffer[1] = DEV_ID;
    radio_buffer[2] = frame_index;
    radio_buffer[3] = packet_count;
    radio_buffer[4] = tx_packet_index;
#ifdef print_packet    
    PRINTF("START PACKET\r\n");
#endif
    if( tx_i == packet_count - 1){
      for( tx_j = HEADER_SIZE; tx_j < last_packet_size; tx_j++ ){
        radio_buffer[tx_j] = frame_data[frame_track + tx_j - HEADER_SIZE];
#ifdef print_packet   
        PRINTF("%u ", radio_buffer[tx_j]);
#endif 
       }
    }else{
      for( tx_j = HEADER_SIZE; tx_j < PACKET_SIZE; tx_j++ ){
        radio_buffer[tx_j] = frame_data[frame_track + tx_j - HEADER_SIZE];
#ifdef print_packet    
        PRINTF("%u ", radio_buffer[tx_j]);
#endif 
      }
    }
#ifdef print_packet   
    PRINTF("\r\nEND PACKET\r\n");
#endif
    __delay_cycles(16000000); 

#ifdef enable_debug    
    PRINTF("Cap ready\n\r");
#endif  
  
    spi_init();

#ifdef enable_debug    
  PRINTF("done with SPI init.\r\n");
#endif

#ifdef OLD_PINS
    P4DIR |= BIT7;
    P4OUT |= BIT7;
#else
    P4DIR |= BIT4;
    P4OUT |= BIT4;
#endif

#ifdef enable_debug    
  PRINTF("done with SPI init.\r\n");
#endif
    camaroptera_init_lora();
#ifdef enable_debug    
  PRINTF("done with lora init.\r\n");
#endif

    if( tx_i == packet_count - 1){
#ifdef enable_debug    
      PRINTF("Sending last packet\n\r");
#endif  
      sx1276_send( radio_buffer,  last_packet_size);
    }else{
#ifdef enable_debug    
      PRINTF("Sending packet %u \r\n",tx_i);
#endif  
      sx1276_send( radio_buffer, PACKET_SIZE );
    }
  
#ifdef enable_debug    
      PRINTF("LPM4_bits + GIE \r\n");
#endif  
    __bis_SR_register(LPM4_bits+GIE);

#ifdef enable_debug    
      PRINTF("dio0irq \r\n");
#endif  
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

#ifndef cont_power
    //Wait to charge up
    charge_rate_sum += camaroptera_wait_for_charge();
#else
    __delay_cycles(80000000);
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
      
#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P2OUT &= ~BIT3;     // tp_status
  P6OUT &= ~BIT7;     // Running: Transmission
#endif
      
  P8OUT &= ~BIT3; 

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

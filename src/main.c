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
//#define print_diff
//#define print_image
//#define print_charging
//#define print_packet
#define print_jpeg

#ifdef enable_debug
  #include <libio/console.h>
#endif

#define __ro_hifram __attribute__((section(".upper.rodata")))
#define __fram __attribute__((section(".persistent")))
//#define OLD_PINS

#define STATE_CAPTURE 0
#define STATE_DIFF 1
#define STATE_INFER 2
#define STATE_COMPRESS 3
#define STATE_TRANSMIT 4

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
#define TX_OUTPUT_POWER                    17        // dBm
#define BUFFER_SIZE                       256 // Define the payload size here

#define MAC_HDR                           0xDF    
#define DEV_ID                            0x04

#define PACKET_SIZE                        200
#define HEADER_SIZE                        5


//Jpeg quality factor
#define JQ LIBJPEG_QF

// Image differencing parameters
#define P_THR 40
#define P_DIFF_THR 400

__ro_hifram uint8_t radio_buffer[BUFFER_SIZE];

__ro_hifram uint16_t High_Threshold = 0x0FFA;   // ~3.004V

extern uint8_t frame[];
extern uint8_t frame_jpeg[];
extern uint16_t fp_track;
extern uint16_t fn_track;

__ro_hifram uint8_t old_frame [19200]= {0};

__ro_hifram pixels = 0;

__ro_hifram uint8_t tx_packet_index = 0;
__ro_hifram uint8_t frame_index = 0;
__ro_hifram uint16_t frame_track = 0;
__fram uint8_t camaroptera_state = 0;
__ro_hifram static radio_events_t radio_events;

__ro_hifram int state = 0;
__fram uint8_t predict;
__ro_hifram int i, j;
__ro_hifram uint16_t packet_count, sent_history, last_packet_size; 
__ro_hifram  uint16_t len = 0;
__ro_hifram jpec_enc_t *e;

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
__ro_hifram int8_t camaroptera_mode_1[5] = {3, -1, -1, 4, 0} ;     // SEND ALL
__ro_hifram int8_t camaroptera_mode_2[5] = {1, 3, -1, 4, 0} ;     // DIFF + SEND
__ro_hifram int8_t camaroptera_mode_3[5] = {1, 2, 3, 4, 0} ;       // DIFF + INFER + SEND
__ro_hifram int8_t camaroptera_mode_4[5] = {3, -1, -1, 0, 0} ;       // DIFF + JPEG
//__ro_hifram int8_t camaroptera_mode_3[5] = {2, 0, 0, 0, 0} ;     // For testing only capture+infer
__ro_hifram int8_t *camaroptera_current_mode = camaroptera_mode_4;
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
extern void task_init();
extern void task_exit();

void camaroptera_capture(){
    
#ifdef EXPERIMENT_MODE
  P5OUT |= BIT6;     // Running: capture
  P5OUT |= BIT5;     // Signal start
#endif


#ifdef enable_debug          
  PRINTF("STATE 0: Capturing a photo.\r\n");
#endif

  pixels = 0;
  while(pixels == 0){
    pixels = capture();
  }

#ifdef EXPERIMENT_MODE
  frame_not_empty_status = P4IN & BIT0;
  frame_interesting_status = P7IN & BIT4;
  //frame_not_empty_status = 1;
  //frame_interesting_status = 1;
#endif


#ifdef print_image
  PRINTF("Captured ---%i--- pixels\r\n", pixels);
  PRINTF("Start captured frame\r\n");
  for( i = 0 ; i < pixels ; i++ ){
    PRINTF("%u ", frame[i]);
  }
  PRINTF("\r\nEnd frame\r\n");
#endif
  //PRINTF("Done Capturing\r\n");
  camaroptera_state = camaroptera_next_task(0);
      
#ifndef cont_power
  camaroptera_wait_for_charge();       //Wait to charge up 
#endif

#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P5OUT &= ~BIT6;     // Running: capture
#endif
      
}

void camaroptera_diff(){
#ifdef enable_debug          
  PRINTF("STATE 1: Performing Diff.\r\n");
#endif

#ifdef EXPERIMENT_MODE
  P6OUT |= BIT4;     // Running: Diff
  P5OUT |= BIT5;     // Signal start
  diff(); // Run diff to simulate workload, but don't use result, diff status read from pins
  if(frame_not_empty_status){   // Denotes an interesting scene
    PRINTF("===>>Scene is interesting.\r\n");
    camaroptera_state = camaroptera_next_task(1);
  }else{
    PRINTF("===>>Scene is empty.\r\n");
    camaroptera_state = 0;
  }
#else //EXPERIMENT_MODE

  if(diff()){
#ifdef enable_debug          
    PRINTF("Frame is different\r\n");
#endif
    camaroptera_state = camaroptera_next_task(1);
  }else{
#ifdef enable_debug          
    PRINTF("No change detected\r\n");
#endif
    camaroptera_state = 0;
  }
#endif //EXPERIMENT_MODE

#ifndef cont_power
  camaroptera_wait_for_charge();       //Wait to charge up 
#endif
      
#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P6OUT &= ~BIT4;     // Running: Diff
#endif
      

}

void camaroptera_infer(){
#ifdef EXPERIMENT_MODE
  P6OUT |= BIT5;     // Running: Infer
  P5OUT |= BIT5;     // Signal start
#endif

#ifdef enable_debug          
  PRINTF("STATE 2: Calling DNN.\r\n");
#endif
  P8OUT ^= BIT1; 

#ifdef USE_ARM_DNN
  P4OUT |= BIT4;
  for(int i = 0; i < 1000; i++){
    __delay_cycles(3040);//TODO: BML: Why this constant?
  }
  P4OUT &= ~BIT4;
  task_exit();
#else
  TRANSITION_TO(task_init);
#endif   

}

void camaroptera_compress(){
#ifdef EXPERIMENT_MODE
  P6OUT |= BIT6;     // Running: Compression
  P5OUT |= BIT5;     // Signal start
#endif
      
#ifdef enable_debug          
  PRINTF("STATE 3: Calling JPEG Compression.\r\n");
#endif


  camaroptera_compression();

#ifdef print_jpeg
  PRINTF("Start JPEG frame\r\n");
  for( i = 0 ; i < len ; i++ ){
    PRINTF("%u ", frame_jpeg[i]);
  }
  PRINTF("\r\nEnd JPEG frame\r\n");
#endif
  camaroptera_state = camaroptera_next_task(3);
      
#ifndef cont_power
  camaroptera_wait_for_charge();       //Wait to charge up 
#endif

#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P6OUT &= ~BIT6;     // Running: Compression
#endif

}

void camaroptera_transmit(){
#ifdef EXPERIMENT_MODE
  if(frame_interesting_status){
    P2OUT |= BIT3;
  }
  pixels = 1800; //TODO: BML: Kill constant
  P6OUT |= BIT7;     // Running: Transmission
  P5OUT |= BIT5;     // Signal start
#endif // EXPERIMENT_MODE      

  charge_rate_sum = 0; 
#ifdef enable_debug    
  PRINTF("STATE 4: Detected person in Image. Calling Radio.\r\n");
#endif

  packet_count = pixels  / (PACKET_SIZE - HEADER_SIZE);

  last_packet_size = pixels - packet_count * (PACKET_SIZE - HEADER_SIZE) + HEADER_SIZE;

  if(pixels % (PACKET_SIZE - HEADER_SIZE) != 0){
    packet_count ++;
  }

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
    }else{
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
  

#ifdef enable_debug    
    PRINTF("Cap ready\n\r");
#endif  
  
    spi_init();

#ifdef OLD_PINS
    P4DIR |= BIT7;
    P4OUT |= BIT7;
#else
    P4DIR |= BIT4;
    P4OUT |= BIT4;
#endif

    camaroptera_init_lora();

    if( i == packet_count - 1){
#ifdef enable_debug    
      PRINTF("Sending packet\n\r");
#endif  
      sx1276_send( radio_buffer,  last_packet_size);
    }else{
      sx1276_send( radio_buffer, PACKET_SIZE );
    }
  
    __bis_SR_register(LPM4_bits+GIE);

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
    //__delay_cycles(80000000);
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

int camaroptera_main(void) {
  PRINTF("Entering main: %u\r\n", camaroptera_state);
  while(1){

    switch( camaroptera_state ){

      case STATE_CAPTURE: //CAPTURE
        camaroptera_capture();
        break;
  
      case STATE_DIFF: //DIFF
        camaroptera_diff();
        break;

      case STATE_INFER: //DNN
        camaroptera_infer();
        break;

      case STATE_COMPRESS: //COMPRESS
        camaroptera_compress(); 
        break;
      
      case STATE_TRANSMIT: //SEND BY RADIO
        camaroptera_transmit();
        break; 

      default:
        camaroptera_state = 0;
        break;
    } // End switch

  } // End while(1)

} // End main()

float camaroptera_wait_for_charge(){
#ifdef enable_debug
  //PRINTF("Waiting for cap to be charged. Going To Sleep\n\r");
#endif
  
  __delay_cycles(80000);

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

    ADC12CTL0 &= ~ADC12ENC;          // Disable conversion before configuring
  ADC12CTL0 |= ADC12SHT0_2 | ADC12ON;   // Sampling time, S&H=16, ADC12 ON
    ADC12CTL1 |= ADC12SHP | ADC12SHS_0 | ADC12CONSEQ_0 ;      // Use ADC12SC to trigger and single-channel

#ifdef OLD_PINS
    ADC12MCTL0 |= ADC12INCH_7 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
#else
    //ADC12MCTL0 |= ADC12INCH_5 | ADC12EOS | ADC12WINC;        // A5 ADC input select; Vref+ = AVCC
    ADC12MCTL0 |= ADC12INCH_6 | ADC12EOS | ADC12WINC;        // A6 ADC input select; Vref+ = AVCC
#endif
  
  /*
     ADC12IER0 |= ADC12IE0;
  ADC12CTL0 |= (ADC12ENC + ADC12SC);                        // Start sampling/conversion
  
  // Do a single conversion before starting
  __bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts
  ADC12CTL0 &= ~ADC12ENC;
  //PRINTF("Done first adc read\r\n");
  

  if( adc_reading >= High_Threshold ){     // Cap fully charged already
    
    ADC12CTL0 &= ~(ADC12ON+ADC12ENC);
    ADC12IER0 &= ~ADC12IE0;
    return 0;
  }
  else{                     // Cap not fully charged
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
    TA0CTL |= TASSEL__ACLK + ID__8 + MC__UP;   // ACLK = 32768kHz, ID=8 => 1 tick = 244.14us

    charge_timer_count = 0;
    
    adc_flag = 1;
    // Wake from this only on ADC12HIIFG
    __bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts
    
#ifndef OLD_PINS
  P2OUT &= ~BIT4;
#endif
    //PRINTF("Done charging\r\n");

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

    return charge_rate;
  // }
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

#ifdef print_jpeg  
  PRINTF("Starting JPEG compression\n\r");
#endif

  e = jpec_enc_new(frame, 160, 120, JQ);

  jpec_enc_run(e, &len);

  pixels = len;
 
#ifdef enable_debug
  PRINTF("Done Compression. New img size: -- %u -- bytes.\r\n", pixels);
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
                                  
#ifdef print_diff
  PRINTF("DIFFERENT PIXELS: %u\r\n", j);
#endif


  if (j >= 400)
    return 1;
  else
    return 0;
}

void camaroptera_wait_for_interrupt(){
  
  P8DIR &= ~BIT0;         // Set as Input
  P8REN |= BIT0;           // Enable Pullups/downs
  P8OUT &= ~BIT0;         // Set as Input Pulldown

  P8IES &= ~BIT0;         // Interrupt on Low->High
  P8IE |= BIT0;           // Enable Interrupt

  __bis_SR_register( LPM4_bits | GIE );

  }

// ============= ISR Routines

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void){
  // Triggered every 75ms
  //P7OUT |= BIT2;
  //P7OUT &= ~BIT2;
  
  TA0CCTL0 &= ~CCIFG;       // Clear Interrupt Flag
  charge_timer_count++;       // Increment total charging time counter
    
  // If called from cap charging routine
  if(adc_flag){
    ADC12IFGR0 &= ~ADC12IFG0;
    ADC12CTL1 |= ADC12SHP | ADC12SHS_0 | ADC12CONSEQ_0 ;      // Use ADC12SC to trigger and single-channel
    ADC12CTL0 |= (ADC12ON + ADC12ENC + ADC12SC);       // Trigger ADC conversion
    while(!(ADC12IFGR0 & ADC12IFG0));       // Wait till conversion over  
    adc_reading = ADC12MEM0;           // Read ADC value
    ADC12CTL0 &= ~ADC12ENC;           // Disable ADC

    if(adc_reading >= High_Threshold){       // Check if charged
      TA0CCTL0 &= ~CCIE;
      TA0CTL &= ~TAIE;  
      __bic_SR_register_on_exit(LPM3_bits | GIE);
    }
  }  
  else if(crash_check_flag){
    // Triggered every 700ms
    
    //P2OUT |= BIT5;
    //P2OUT &= ~BIT5;
    
    TA0CCTL0 &= ~CCIFG;       // Clear Interrupt Flag    
    crash_flag = 1;
    TA0CCTL0 &= ~CCIE;
    TA0CTL &= ~TAIE;  
    __bic_SR_register_on_exit(LPM0_bits | GIE);
  }  
}
/*
void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12ISR (void){

    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__ADC12HIIFG:          // Vector  6:  ADC12BHI
      ADC12IFGR2 &= ~ADC12HIIFG;
      adc_reading = ADC12MEM0;
      __bic_SR_register_on_exit(LPM3_bits | GIE);
          break;
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0 Interrupt
          ADC12IFGR0 &= ~(ADC12IFG0);
      adc_reading = ADC12MEM0;
      PRINTF("===%u\r\n", adc_flag);
      if(!adc_flag){
        adc_flag = 1;
        __bic_SR_register_on_exit(LPM3_bits | GIE);
        }
            break;
        default: break;
    }
}
*/

void __attribute__ ((interrupt(PORT8_VECTOR))) port_8 (void) {
    P8IE &= ~BIT0;
    P8IFG &= ~BIT0;
    //__bic_SR_register_on_exit(LPM4_bits+GIE);
}


void __attribute__ ((interrupt(PORT5_VECTOR))) port_5 (void) {
    switch(__even_in_range(P5IV, P5IV__P5IFG7))
    {
        case P5IV__NONE:    break;          // Vector  0:  No interrupt
        case P5IV__P5IFG0:  break;          // Vector  2:  P7.0 interrupt flag
        case P5IV__P5IFG1:                  // Vector  4:  P7.1 interrupt flag
            break;
        case P5IV__P5IFG2:          // Vector  6:  P7.2 interrupt flag
            break;
        case P5IV__P5IFG3:          // Vector  8:  P7.3 interrupt flag
            break;
        case P5IV__P5IFG4:                // Vector  10:  P7.4 interrupt flag
      break;
        case P5IV__P5IFG5:  break;          // Vector  12:  P7.5 interrupt flag
        case P5IV__P5IFG6:  break;          // Vector  14:  P7.6 interrupt flag
        case P5IV__P5IFG7:  // Vector  16:  P7.7 interrupt flag
      P5IFG &= ~BIT7; 
        
      //fp_track = 0;
      //fn_track = 0;
      PRINTF("==== Reset fp/fn tracks\r\n");
      break;
        default: break;
    }
}


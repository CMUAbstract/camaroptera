#ifndef CAMAROPTERA
  #define CAMAROPTERA
#endif

#include <stdint.h>
#include <stdio.h>
#include <libio/console.h>
#include <libhimax/hm01b0.h>
#include <libmsp/gpio.h>
#include <libmsp/mem.h>
#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>

#include "cam_lora.h"
#include "cam_util.h"
#include "cam_diff.h"
#include "cam_radio.h"
#include "cam_compress.h"
#include "cam_capture.h"
#include "cam_framebuffer.h"
#include "cam_process.h"
#include "cam_interrupt.h"


#ifdef enable_debug
  #include <libio/console.h>
#endif


/*Capture-related data*/

/*Game-loop data*/
__fram uint8_t camaroptera_state = 0;

/*BML: used to decide when to switch mode state machines*/ 
__ro_hifram float threshold_1 = 20.0;
__ro_hifram float threshold_2 = 100.0;
extern uint8_t prediction;

/*TODO: define meaningful constants for these states*/
// Different Operating modes ==> Next task ID for tasks {0,1,2,3,4}
// [0] - Capture Image
// [1] - Diff
// [2] - Infer
// [3] - Compress
// [4] - Send Packet
__ro_hifram int8_t camaroptera_mode_1[5] = {3, -1, -1, 4, 0} ;    // SEND ALL
__ro_hifram int8_t camaroptera_mode_2[5] = {1, 3, -1, 4, 0} ;     // DIFF + SEND
__ro_hifram int8_t camaroptera_mode_3[5] = {1, 2, 3, 4, 0} ;      // DIFF + INFER + SEND
__ro_hifram int8_t camaroptera_mode_4[5] = {3, -1, -1, 0, 0} ;    // DIFF + JPEG
__ro_hifram int8_t camaroptera_mode_5[5] = {2, -1, 3, 4, 0} ;     // DIFF + INFER + SEND
__ro_hifram int8_t camaroptera_mode_6[5] = {4, -1, -1, -1, 0} ;   // DIFF + INFER + SEND
__ro_hifram int8_t camaroptera_mode_7[5] = {0, -1, -1, -1, -1} ;  // CAPTURE Only
__ro_hifram int8_t camaroptera_mode_8[5] = {2, -1, 0, -1, -1} ;   // CAPTURE + INFER
__ro_hifram int8_t camaroptera_mode_9[5] = {2, -1, 3, 4, 0} ;     // CAPTURE + INFER + COMPRESS + SEND
__ro_hifram int8_t camaroptera_mode_10[5] = {2, -1, 4, -1, 0} ;   // CAPTURE + INFER + SEND
__ro_hifram int8_t camaroptera_mode_11[5] = {3, -1, -1, 4, 0} ;   // CAPTURE + COMPRESS + SEND
__ro_hifram int8_t *camaroptera_current_mode = camaroptera_mode_7;

#ifndef CONFIG_CONSOLE
#ifdef enable_debug          
  #pragma message "no console"
#endif
  #define printf(fmt, ...) (void)0
#endif

static void init_hw() {
  msp_watchdog_disable();
  msp_gpio_unlock();
  msp_clock_setup();
}

void init() {

  init_hw();
  INIT_CONSOLE();

  __enable_interrupt();

  P8OUT &= ~(BIT1+BIT2+BIT3);
  P8DIR |= (BIT1+BIT2+BIT3);
  
#ifdef EXPERIMENT_MODE
  
  // Set as input
  P4DIR &= ~BIT0;
  P7DIR &= ~(BIT4);

  // Disable Pullup/downs
  P4REN &= ~BIT0;
  P7REN &= ~BIT4;

  P2OUT &= ~BIT3;
  P2DIR |= BIT3;
  
  P5OUT &= ~(BIT5+BIT6);
  P5DIR |= (BIT5+BIT6);
  P6OUT &= ~(BIT4+BIT5+BIT6+BIT7);
  P6DIR |= (BIT4+BIT5+BIT6+BIT7);
  
  // External Peripheral Power EN
  P4OUT &= ~BIT4;
  P4DIR |= BIT4;

#endif
  

}


int main(void) {

  init();

  PRINTF("HELLO FROM CAMAROPTERA MAIN\r\n");
  P2DIR |= 0xff;
  while(1){
    P2OUT ^= BIT1;
    //P2SELC &= ~(BIT0 | BIT1);
    __delay_cycles(16000000);

  } // End while(1)

} // End main()





// float camaroptera_wait_for_charge(){
// #ifdef cont_power
// return;
// #endif

// #ifdef enable_debug
//   //PRINTF("Waiting for cap to be charged. Going To Sleep\n\r");
// #endif
  
//   __delay_cycles(80000);

// #ifdef OLD_PINS
//     P2SEL0 |= BIT4;                                 //P1.0 ADC mode
//     P2SEL1 |= BIT4;                                 //
// #else
//     //P1SEL0 |= BIT5;                                 //P1.0 ADC mode
//     //P1SEL1 |= BIT5;                                 //
//     P2SEL0 |= BIT3;                                 //P2.3 ADC mode
//     P2SEL1 |= BIT3;                                 //
//   P2DIR |= BIT4;
//   P2OUT |= BIT4;
// #endif
    
//   // ======== Configure ADC ========
//   // Take single sample when timer triggers and compare with threshold

//     ADC12CTL0 &= ~ADC12ENC;          // Disable conversion before configuring
//   ADC12CTL0 |= ADC12SHT0_2 | ADC12ON;   // Sampling time, S&H=16, ADC12 ON
//     ADC12CTL1 |= ADC12SHP | ADC12SHS_0 | ADC12CONSEQ_0 ;      // Use ADC12SC to trigger and single-channel

// #ifdef OLD_PINS
//     ADC12MCTL0 |= ADC12INCH_7 | ADC12EOS | ADC12WINC;        // A7 ADC input select; Vref+ = AVCC
// #else
//     //ADC12MCTL0 |= ADC12INCH_5 | ADC12EOS | ADC12WINC;        // A5 ADC input select; Vref+ = AVCC
//     ADC12MCTL0 |= ADC12INCH_6 | ADC12EOS | ADC12WINC;        // A6 ADC input select; Vref+ = AVCC
// #endif
      
//        uint16_t initial_voltage = adc_reading;
//     uint16_t voltage_temp;  

//     ADC12IER0 &= ~ADC12IE0;
//     ADC12HI = High_Threshold;                               // Enable ADC interrupt
//       ADC12IER2 &= ~ADC12HIIE;                                  // Enable ADC threshold interrupt
  
//     // ========= Configure Timer =======
//     // Timer = 205 = ~50ms
    
//     TA0CTL |= TACLR;
//     TA0CCR0 = 307; 
//     TA0CCTL0 |= CCIE;
//     TA0CTL |= TASSEL__ACLK + ID__8 + MC__UP;   // ACLK = 32768kHz, ID=8 => 1 tick = 244.14us

//     charge_timer_count = 0;
    
//     adc_flag = 1;
//     // Wake from this only on ADC12HIIFG
//     __bis_SR_register(LPM3_bits | GIE);                     // Enter LPM3, enable interrupts
    
// #ifndef OLD_PINS
//   P2OUT &= ~BIT4;
// #endif
//     //PRINTF("Done charging\r\n");

//     uint32_t timer_temp = 0;
//     timer_temp = (charge_timer_count*TA0CCR0) + TA0R;
    
//     TA0CCTL0 &= ~CCIE;
//     TA0CTL &= ~TAIE;
//     TA0CTL |= TACLR;
//     TA0CTL |= MC__STOP;
//     ADC12CTL0 &= ~(ADC12ON+ADC12ENC);
//     ADC12IER2 &= ~ADC12HIIE;
//     ADC12IER0 &= ~ADC12IE0;

// #ifdef print_charging
//     PRINTF("Timer Value After: (HI)%u", (timer_temp>>16));
//     PRINTF("(LO)%u\r\n", (timer_temp & 0xFFFF)) ;
// #endif
    
//     voltage_temp = adc_reading - initial_voltage;

// #ifdef print_charging
//     PRINTF("Capacitor Charge Value Changed: %i\r\n", voltage_temp);
// #endif
  
//     int32_t temp = voltage_temp * 10;
//     timer_temp = timer_temp / 1000;
//     float charge_rate = temp / timer_temp;
// #ifdef print_charging
//     PRINTF("\r\nCHARGE RATE: %i\r\n", (int)charge_rate);
// #endif

//     return charge_rate;
//   // }
// }



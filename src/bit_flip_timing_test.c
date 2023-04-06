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
#include "power_analysis.h"


__ro_hifram uint16_t High_Threshold = ADC_3_004V;   
__ro_hifram volatile uint8_t charge_timer_count;
__ro_hifram volatile uint16_t adc_reading;
__ro_hifram uint8_t adc_flag;
__fram volatile uint8_t crash_flag;
__ro_hifram volatile uint8_t crash_check_flag;




void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void){
  // Triggered every 75ms
  //P7OUT |= BIT2;
  //P7OUT &= ~BIT2;
  P2OUT ^= BIT0 | BIT1;
  
  TA0CCTL0 &= ~CCIFG;       // Clear Interrupt Flag
  charge_timer_count++;       // Increment total charging time counter
    
}



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

//------------------------------------------------------------------------------
// DMA interrupt handler
//------------------------------------------------------------------------------
void __attribute__ ((interrupt(DMA_VECTOR))) DMA_ISR (void){

  DMA0CTL &= ~DMAIFG;                       // Clear DMA0 interrupt flag
  __bic_SR_register_on_exit(LPM0_bits+GIE);     // Exit LPM0

}



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

  init_hw();
  //INIT_CONSOLE();

  P2SEL0 &= ~(BIT0 | BIT1);                                 //Select RX/TX
  P2SEL1 &= ~(BIT0 | BIT1);                                 //
  P2DIR |= BIT0 | BIT1;
  

  // CONFIGURE SMCLK

  // CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
  // CSCTL1 = BIT3 + BIT6;                       // Set DCO to 16MHz
  // CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK; // set ACLK = XT1; MCLK = DCO
  // CSCTL3 = DIVA__1 | DIVS__1 | DIVM__2;     // Set all dividers
  // CSCTL4 &= ~LFXTOFF;


    // ========= Configure Timer =======
    // Timer = 1999 = ~1ms (996us last cal)

    TA0CTL |= TACLR;
    TA0CCR0 = 1999; 
    TA0CCTL0 |= CCIE;
    TA0CTL |= TASSEL__SMCLK + ID__1 + MC__UP;   // SMCLK, ID=2 => 1 tick = 1/16MHz => .0625us

    charge_timer_count = 0;
    
    adc_flag = 1;
    _enable_interrupts();                     // Enter LPM3, enable interrupts
    

    uint32_t timer_temp = 0;
    
  while(1){
    //__delay_cycles(100000);
    // timer_temp = (charge_timer_count*TA0CCR0) + TA0R;
    // PRINTF("timer count: %i\r\n",charge_timer_count);
    // PRINTF("ta_val: %i\r\n",TA0CCR0);
    // PRINTF("ta0r: %i\r\n",TA0R);
    // PRINTF("timer: %i\r\n",timer_temp);
    //__delay_cycles(8000);

    

		// PRINTF("Timer Value After: (HI)%u", (timer_temp>>16));
		// PRINTF("(LO)%u\r\n", (timer_temp & 0xFFFF)) ;
		// if(charge_timer_count % 4==0){
    //   PRINTF("Capacitor dv/dt: %i\r\n", sum/(charge_timer_count+1));
      
    //   sum = 0;
    // }
	
		// int32_t temp = voltage_temp * 10;
		// timer_temp = timer_temp / 1000;
		// float charge_rate = temp / timer_temp;
		// PRINTF("\r\nCHARGE RATE: %i\r\n", (int)charge_rate);





    //SAVE TO MEM BEGIN
    //Block size
    // DMA0SZ = i - (raw_W - W);

    //       // DMA Block repeat + Src inc + Dst incr + Src byte + Dst Byte + DMA int
    //       DMA0CTL = DMADT_5 | DMASRCINCR_3 | DMADSTINCR_3 | DMASRCBYTE | DMADSTBYTE | DMAIE;  

    //     // DMA enable                              
    //     DMA0CTL |= DMAEN;    

    //     //PRINTF("FRAM array address incremented: %u\n\r", DMA0DA);

    //     // Manual transfer trigger
    //     DMA0CTL |= DMAREQ;   

    // SAVE TO MEM END

  } // End while(1)

  TA0CCTL0 &= ~CCIE;
    TA0CTL &= ~TAIE;
    TA0CTL |= TACLR;
    TA0CTL |= MC__STOP;
    ADC12CTL0 &= ~(ADC12ON+ADC12ENC);
    ADC12IER2 &= ~ADC12HIIE;
    ADC12IER0 &= ~ADC12IE0;

} // End main()


#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>

#include <libmsp/mem.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include "cam_util.h"
#include "cam_interrupt.h"

__ro_hifram uint16_t High_Threshold = ADC_3_004V;   
__ro_hifram volatile uint8_t charge_timer_count;
__ro_hifram volatile uint16_t adc_reading;
__ro_hifram uint8_t adc_flag;
__fram volatile uint8_t crash_flag;
__ro_hifram volatile uint8_t crash_check_flag;

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

float camaroptera_wait_for_charge(){
#ifdef cont_power
return;
#endif

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

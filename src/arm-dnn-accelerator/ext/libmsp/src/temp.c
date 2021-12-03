#include <msp430.h>

#include <libio/console.h>

#include "temp.h"

// Table 6-62: temp calibration for 1.2v reference
#define TLV_CAL30 ((int *)(0x01A1A))
#define TLV_CAL85 ((int *)(0x01A1C))

#define FIXED_POINT_FACTOR 10 // units are (1 / FIXDPOINT_FACTOR) degrees C

// Returns temperature in degrees C (approx range -40deg - 85deg)
int msp_sample_temperature() {
  ADC12CTL0 &= ~ADC12ENC;           // Disable conversions

  ADC12CTL3 |= ADC12TCMAP;
  ADC12CTL1 = ADC12SHP;
  ADC12CTL2 = ADC12RES_2;
  ADC12MCTL0 = ADC12VRSEL_1 | BIT4 | BIT3 | BIT2 | BIT1;
  ADC12CTL0 |= ADC12SHT03 | ADC12ON;

  while( REFCTL0 & REFGENBUSY );

  REFCTL0 = REFVSEL_0 | REFON;

  // Wait for REF to settle
  // TODO: to use msp_sleep, we need to take the (clock+divider) as arg
  __delay_cycles(PERIOD_LIBMSP_REF_SETTLE_TIME);
  //msp_sleep(PERIOD_LIBMSP_REF_SETTLE_TIME);

  ADC12CTL0 |= ADC12ENC;                  // Enable conversions
  ADC12CTL0 |= ADC12SC;                   // Start conversion
  ADC12CTL0 &= ~ADC12SC;                  // We only need to toggle to start conversion
  while (ADC12CTL1 & ADC12BUSY);

  int sample = ADC12MEM0;

  ADC12CTL0 &= ~ADC12ENC;           // Disable conversions
  ADC12CTL0 &= ~(ADC12ON);  // Shutdown ADC12
  REFCTL0 &= ~REFON;

  int cal30 = *TLV_CAL30;
  int cal85 = *TLV_CAL85;
  int tempC = ((int32_t)(sample - cal30) * 55 * FIXED_POINT_FACTOR) / (cal85 - cal30) + (30 * FIXED_POINT_FACTOR);

  LOG2("[temp] sample=%i => T=%i\r\n", sample, tempC);

  return tempC;
}

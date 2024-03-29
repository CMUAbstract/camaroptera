#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>


#include <libmsp/mem.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include "camaroptera-dnn.h"


void camaroptera_infer(){
#ifdef EXPERIMENT_MODE
  P6OUT |= BIT5;     // Running: Infer
  P5OUT |= BIT5;     // Signal start
#endif

#ifdef enable_debug          
  PRINTF("STATE 2: Calling DNN.\r\n");
#endif

  P8OUT ^= BIT1; 

#ifdef enable_debug          
  PRINTF("after p8 stuff.\r\n");
#endif

#ifdef USE_ARM_DNN
  P4OUT |= BIT4;
  for(int i = 0; i < 1000; i++){
    __delay_cycles(3040);//TODO: BML: Why this constant?
  }
  P4OUT &= ~BIT4;
  task_exit();
#else
#ifdef enable_debug          
  PRINTF("transitioning to task_init.\r\n");
#endif

task_init();
task_compute();
task_finish();
task_exit();

#endif   

}

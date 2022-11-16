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
__ro_hifram int8_t *camaroptera_current_mode = camaroptera_mode_3;

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

#ifdef CONFIG_CONSOLE
  INIT_CONSOLE();
#endif

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
  
#ifndef cont_power
  camaroptera_wait_for_charge();       //Wait to charge up 
#endif

}


int main(void) {

  init();

  PRINTF("HELLO FROM CAMAROPTERA MAIN\r\n");

  camaroptera_init_framebuffer();
  hm01b0_set_framebuffer( camaroptera_get_framebuffer() );

  PRINTF("Entering main: %u\r\n", camaroptera_state);
  while(1){

    uint16_t num_pixels = 0;
    switch( camaroptera_state ){

      case STATE_CAPTURE: //CAPTURE
        //TODO: validate that this works on real HW -- swap may break hm01b0 
        //camaroptera_swap_framebuffer_dbl_buf(); 
        //hm01b0_set_framebuffer( camaroptera_get_framebuffer() );

        camaroptera_capture();
        break;
  
      case STATE_DIFF: //DIFF
        camaroptera_diff(P_THR);
        break;

      case STATE_INFER: //STATE_INFER used as a placeholder for generic processing
        camaroptera_process();
        prediction = 0xAB; 	// Set to dummy value if not used
		break;

      case STATE_COMPRESS: //COMPRESS
        num_pixels = camaroptera_compress();
        camaroptera_set_framebuffer_num_pixels(num_pixels);
        break;

      case STATE_TRANSMIT: //SEND BY RADIO
#ifndef send_label_only
        num_pixels = camaroptera_get_framebuffer_num_pixels();
#else
        num_pixels = 1;
#endif
        camaroptera_transmit(num_pixels, prediction);
        break; 

      default:
        camaroptera_state = STATE_CAPTURE;
        break;
    } // End switch

  } // End while(1)

} // End main()



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
  
  if ( camaroptera_current_mode[current_task] == -1 )
    return 0;
  else
    return camaroptera_current_mode[current_task];
}



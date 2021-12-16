#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>

#include <libmsp/mem.h>
#include <libmsp/gpio.h>

#include <libhimax/hm01b0.h>

#include "cam_util.h"
#include "cam_capture.h"
#include "cam_framebuffer.h"

/*Experimental instrumentation*/
#ifdef EXPERIMENT_MODE
__ro_hifram uint8_t image_capt_not_sent = 0;
__ro_hifram uint8_t frame_not_empty_status, frame_interesting_status;
#endif

void camaroptera_capture(){
    
#ifdef EXPERIMENT_MODE
  P5OUT |= BIT6;     // Running: capture
  P5OUT |= BIT5;     // Signal start
#endif


#ifdef enable_debug          
  PRINTF("STATE 0: Capturing a photo.\r\n");
#endif

  uint16_t captured_pixels = 0;
  while(captured_pixels == 0){
    captured_pixels = capture();
  }
  camaroptera_set_framebuffer_num_pixels(captured_pixels);

#ifdef EXPERIMENT_MODE
  frame_not_empty_status = P4IN & BIT0;
  frame_interesting_status = P7IN & BIT4;
  //frame_not_empty_status = 1;
  //frame_interesting_status = 1;
#endif

#ifdef print_image
  PRINTF("Captured ---%i--- pixels\r\n", captured_pixels);
  PRINTF("Start captured frame\r\n");
  uint8_t *frame = camaroptera_get_framebuffer();
  for( int i = 0 ; i < captured_pixels ; i++ ){
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

#ifndef CAMAROPTERA
  #define CAMAROPTERA
#endif

#include <stdint.h>
#include <stdio.h>
#include <libio/console.h>

#include "cam_lora.h"
#include "cam_util.h"
#include "cam_diff.h"
#include "cam_radio.h"
#include "cam_compress.h"
#include "cam_capture.h"
#include "cam_infer.h"

#ifdef enable_debug
  #include <libio/console.h>
#endif

extern uint8_t old_frame[];

/*Capture-related data*/
/*TODO: BML: why old_frame with fixed pixels but frame not?*/
__ro_hifram size_t pixels = 0;

/*Game-loop data*/
__fram uint8_t camaroptera_state = 0;

/*BML: used to decide when to switch mode state machines*/ 
__ro_hifram float threshold_1 = 20.0;
__ro_hifram float threshold_2 = 100.0;

/*TODO: define meaningful constants for these states*/
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
__ro_hifram int8_t *camaroptera_current_mode = camaroptera_mode_1;


int camaroptera_main(void) {
  PRINTF("Entering main: %u\r\n", camaroptera_state);
  while(1){

    switch( camaroptera_state ){

      case STATE_CAPTURE: //CAPTURE
        camaroptera_capture();
        break;
  
      case STATE_DIFF: //DIFF
        camaroptera_diff(frame,old_frame,pixels,P_THR);
        break;

      case STATE_INFER: //DNN
        camaroptera_infer();
        break;

      case STATE_COMPRESS: //COMPRESS
        pixels = camaroptera_compress(); 
        break;
      
      case STATE_TRANSMIT: //SEND BY RADIO
        camaroptera_transmit(pixels);
        break; 

      default:
        camaroptera_state = 0;
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



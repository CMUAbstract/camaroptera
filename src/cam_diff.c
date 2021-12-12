#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cam_util.h"
#include "cam_diff.h"

__ro_hifram uint8_t old_frame[FRAME_PIXELS] = {0}; 

void camaroptera_diff(uint8_t *frame, uint8_t *old_frame,size_t pixels, uint8_t thresh){

#ifdef EXPERIMENT_MODE
  P6OUT |= BIT4;     // Running: Diff
  P5OUT |= BIT5;     // Signal start
  cam_diff(); // Run diff to simulate workload, but don't use result, diff status read from pins
  if(frame_not_empty_status){   // Denotes an interesting scene
    camaroptera_state = camaroptera_next_task(1);
  }else{
    camaroptera_state = 0;
  }
#else //EXPERIMENT_MODE
  /*TODO: make these args and arg to this function and then call from main*/
  if(cam_diff(frame,old_frame,pixels,thresh)){
    camaroptera_state = camaroptera_next_task(1);
  }else{
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


uint8_t cam_diff(uint8_t *newf, uint8_t *oldf, size_t size, uint8_t thresh){

  uint16_t i, j = 0;
  for(i = 0; i < size; i++){
    if ((newf[i] - oldf[i] >= thresh) || (oldf[i] - newf[i] >= thresh))
      j++;
  }
                            
  /*TODO: zero-copy double buffer*/  
  memcpy(oldf, newf, sizeof(oldf));

  if (j >= 400)
    return 1;
  else
    return 0;
}

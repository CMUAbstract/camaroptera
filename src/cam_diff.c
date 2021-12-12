#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "cam_util.h"
#include "cam_diff.h"
#include "cam_framebuffer.h"

void camaroptera_diff(uint8_t thresh){

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
  uint16_t diff_result = 
    cam_diff(camaroptera_get_framebuffer(),
             camaroptera_get_framebuffer_dbl_buf(),
             camaroptera_get_framebuffer_num_pixels(),
             thresh);
  if( diff_result ){
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
  //memcpy(oldf, newf, sizeof(oldf));
  camaroptera_swap_framebuffer_dbl_buf();

  if (j >= 400)
    return 1;
  else
    return 0;
}

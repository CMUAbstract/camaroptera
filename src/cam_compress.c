#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include <libio/console.h>

#include <libmsp/mem.h>
#include <libmsp/gpio.h>

#include <libjpeg/jpec.h> 

#include "lzfx.h"

#include "cam_util.h"
#include "cam_framebuffer.h"
#include "cam_compress.h"


#ifndef cam_downsamp
/*JPEG Compression-related data*/
__ro_hifram jpec_enc_t *e; 
/*TODO: do you want the pointer to this NV?  Or the thing?*/
#else

#define CAM_DOWNSAMP_SIZE 4800
__fram uint8_t cam_downsamp_buf[CAM_DOWNSAMP_SIZE];

void cam_downsample(uint8_t *buf){
  int outpx = 0;
  for( int i = 0; i < 120; i++ ){ 
    for(int j = 0; j < 160; j++){ 
 
      if( i % 2 == 0 && j % 2 == 0){
        cam_downsamp_buf[outpx] = buf[i*160 + j];
        outpx++;
      }
    }
  }
}

#endif


uint16_t camaroptera_compression(){

#ifdef print_jpeg  
  PRINTF("Starting compression\n\r");
#endif

#ifndef cam_downsamp
  /*TODO: Does this dynamically allocate memory? If so, do statically?*/
  e = jpec_enc_new(camaroptera_get_framebuffer(), 160, 120, JQ);

  int len = 0;
  jpec_enc_run(e, (int*)&len);
#else

#ifdef enable_debug
  PRINTF("Compress: calling lzfx");
#endif

  cam_downsample(camaroptera_get_framebuffer()); 
  unsigned int len = CAM_DOWNSAMP_SIZE; 

#endif

 
#ifdef enable_debug
  PRINTF("Done Compression. New img size: -- %u -- bytes.\r\n", camaroptera_get_framebuffer_num_pixels());
#endif
  return len;

}

uint16_t camaroptera_compress(){
#ifdef EXPERIMENT_MODE
  P6OUT |= BIT6;     // Running: Compression
  P5OUT |= BIT5;     // Signal start
#endif
      
#ifdef enable_debug          
  PRINTF("STATE 3: Calling JPEG Compression.\r\n");
#endif

  uint16_t len = camaroptera_compression();

#ifdef print_jpeg
  PRINTF("Start JPEG frame\r\n");
  for( int i = 0 ; i < len ; i++ ){

#ifndef cam_downsamp
    PRINTF("%u ", frame_jpeg[i]);
#else
    PRINTF("%u ", cam_downsamp_buf[i]);
#endif

  }
  PRINTF("\r\nEnd JPEG frame\r\n");
#endif /*print_jpeg*/
  camaroptera_state = camaroptera_next_task(3);
      
#ifndef cont_power
  camaroptera_wait_for_charge();       //Wait to charge up 
#endif

#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P6OUT &= ~BIT6;     // Running: Compression
#endif
  
  return len;
}

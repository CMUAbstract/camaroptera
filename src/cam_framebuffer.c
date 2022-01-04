#include <stdint.h>
#include <stddef.h>

#include <libhimax/hm01b0.h>

#include "cam_util.h"

__hifram uint8_t frame_dbl_buf0[HM01B0_FB_SIZE];
//__hifram uint8_t frame_dbl_buf1[HM01B0_FB_SIZE];
__hifram uint8_t *this_frame;
__hifram uint8_t *last_frame;
__ro_hifram size_t pixels = 0;

uint8_t *camaroptera_get_framebuffer(){
  return this_frame;
}

uint8_t *camaroptera_get_framebuffer_dbl_buf(){
  return last_frame;
}

void camaroptera_init_framebuffer(){
  this_frame = frame_dbl_buf0;
  //last_frame = frame_dbl_buf1;
}

void camaroptera_swap_framebuffer_dbl_buf(){
  uint8_t *t = last_frame;
  last_frame = this_frame;
  this_frame = t;
}

size_t camaroptera_get_framebuffer_num_pixels(){
  return pixels;
}

void camaroptera_set_framebuffer_num_pixels(size_t num_pixels){
  pixels = num_pixels;
}

#pragma once
uint8_t *camaroptera_get_framebuffer();
uint8_t *camaroptera_get_framebuffer_dbl_buf();
void camaroptera_swap_framebuffer_dbl_buf();
void camaroptera_init_framebuffer();
size_t camaroptera_get_framebuffer_num_pixels();
void camaroptera_set_framebuffer_num_pixels(size_t num_pixels);

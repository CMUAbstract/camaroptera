#include <libfixed/fixed.h>


uint16_t nb = 19200;
uint16_t __attribute__ ((section(".upper.rodata"))) hist8x8[2700];
fixed __attribute__ ((section(".upper.rodata"))) hist16x16[9576];
uint16_t __attribute__ ((section(".upper.rodata"))) g[19200];
fixed __attribute__ ((section(".upper.rodata"))) theta[19200];

void sobel(uint8_t height, uint8_t width);
void histogram(uint8_t height, uint8_t width);
double infer();

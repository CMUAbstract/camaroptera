#include <libfixed/fixed.h>

#define __nv __nvram
#define __nvram __attribute__((section(".upper.rodata")))

__nvram fixed g[19200];
__nvram fixed theta[19200];
__nvram fixed hist[300][9];
__nvram fixed histnorm[300][36];

void sobel(fixed *src, fixed *grad, fixed *angle, uint8_t rows, uint8_t cols);

uint16_t histogram(fixed *src, fixed *dest, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols);

fixed infer(fixed *input);

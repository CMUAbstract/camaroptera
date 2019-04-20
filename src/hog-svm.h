#include <libfixed/fixed.h>

#define __nv __nvram
#define __nvram __attribute__((section(".upper.rodata")))

__nvram uint16_t g[19200];
__nvram uint16_t theta[19200];
__nvram uint16_t hist[2700];
__nvram fixed histnorm[9567];

void sobel(fixed *src, uint16_t *grad, uint16_t *angle, uint8_t rows, uint8_t cols);

void histogram(uint16_t *grad, uint16_t *angle, uint16_t *hist, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets);

void normalize(uint16_t *src, fixed *dest, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets);

fixed infer(fixed *src);

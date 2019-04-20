#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <msp430.h>
#include "hog-svm.h"
#include "svm.h"
// #include "input.h"
#include "test/input_Time0_P2.h"

#include <libfixed/fixed.h>
#include <libio/console.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

int main(void){

	msp_watchdog_disable();

	msp_gpio_unlock();

	msp_clock_setup();
	INIT_CONSOLE();

	P8OUT &= ~BIT0; 
	P8DIR |= BIT0;

	PRINTF("Start\r\n");
	P8OUT |= BIT0;
	sobel(frame, g, theta, 120, 160);
	P8OUT &= ~BIT0; 

	PRINTF("Computing histogram\r\n");
	P8OUT |= BIT0;
	histogram(g, theta, hist, 120, 160, 8, 8, 9);
	P8OUT &= ~BIT0; 
	
	PRINTF("Normalizing\r\n");
	P8OUT |= BIT0;
	normalize(hist, histnorm, 14, 19, 2, 2, 9);
	P8OUT &= ~BIT0; 
	
	PRINTF("Inferring\r\n");
	P8OUT |= BIT0;
	fixed result = infer(histnorm);
	P8OUT &= ~BIT0; 
	
	PRINTF("%i\r\n", result);
	
	if( result < 0 )
		PRINTF("PERSON DETECTED !\r\n");
	else
		PRINTF("No Person :(\r\n");

	return 0;
}

void dump(fixed *src, uint8_t rows, uint8_t cols) {
	uint16_t block_size = 18;
	for(uint8_t i = 0; i < block_size; i++) {
		for(uint8_t j = 0; j < block_size; j++) {
			PRINTF("%i ", *src++);	
		}
		src = src - block_size + cols;
		PRINTF("\r\n");
	}
}

uint32_t i_sqrt(uint32_t x) {
	if (x == 0 || x == 1) return x; 
	int i = 1, result = 1; 
	while(result <= x) { 
		i++; 
		result = i * i; 
	}
	return i - 1; 
}

uint8_t atan_lookup(fixed x) {
	if( x >= F_LIT(0.0) && x < F_LIT(0.36)) return 0;
	else if( x >= F_LIT(0.36) && x < F_LIT(0.84)) return 1;
	else if( x >= F_LIT(0.84) && x < F_LIT(1.73)) return 2;
	else if( x >= F_LIT(1.73) && x < F_LIT(5.67)) return 3;
	else if( x < F_LIT(-5.67) || x >= F_LIT(5.67)) return 4;
	else if( x >= F_LIT(-5.67) && x < F_LIT(-1.73)) return 5;
	else if( x >= F_LIT(-1.73) && x < F_LIT(-0.84)) return 6;
	else if( x >= F_LIT(-0.84) && x < F_LIT(-0.36)) return 7;
	else if( x >= F_LIT(-0.36) && x < F_LIT(0)) return 8;
	return 0;
}

fixed interp(fixed min, fixed max, fixed v) {
	return F_DIV(F_SUB(v, min), F_SUB(max, min));
}

uint8_t _lookup(fixed min, fixed max, fixed v, uint16_t idx) {
	if(v >= min && v < max) return F_ADD((idx << F_N), interp(min, max, v));
	return 0;
}

fixed atan_lookup_interp(fixed v) {
	fixed x;
	if((x = _lookup(F_LIT(0), F_LIT(0.36), v, 0))) return x;
	else if((x = _lookup(F_LIT(0.36), F_LIT(0.84), v, 1))) return x;
	else if((x = _lookup(F_LIT(0.84), F_LIT(1.73), v, 2))) return x;
	else if((x = _lookup(F_LIT(1.73), F_LIT(5.67), v, 3))) return x;
	else if(v > F_LIT(5.67) || v <= F_LIT(5.67)) return F_LIT(4);
	// else if(v > F_LIT(5.67)) return F_ADD(F_LIT(4), F_SUB(v, F_LIT(5.67)));
	// else if(v <= F_LIT(-5.67)) return F_ADD(F_LIT(4), F_ADD(v, F_LIT(5.67)));
	else if((x = _lookup(F_LIT(-5.67), F_LIT(-1.73), v, 5))) return x;
	else if((x = _lookup(F_LIT(-1.73), F_LIT(-0.84), v, 7))) return x;
	else if((x = _lookup(F_LIT(-0.84), F_LIT(-0.36), v, 6))) return x;
	else if((x = _lookup(F_LIT(-0.36), F_LIT(0), v, 8))) return x;
	return 0;
}

void sobel(uint8_t *src, uint16_t *grad, fixed *angle, uint8_t rows, uint8_t cols) {
	// First row
	uint8_t *src_ptr = src + 1;
	uint16_t *grad_ptr = grad + 1;
	fixed *angle_ptr = angle + 1;
	for(uint8_t i = 1; i < cols - 1; i++) {
		*grad_ptr = 0;
		*angle_ptr = 0; 
		src_ptr++;
		grad_ptr++;
		angle_ptr++;
	}

	// First column
	src_ptr = src + cols;
	grad_ptr = grad + cols;
	angle_ptr = angle + cols;
	for(uint8_t i = 1; i < rows - 1; i++) {
		*grad_ptr = 0;
		*angle_ptr = 0;
		src_ptr += cols;
		grad_ptr += cols;
		angle_ptr += cols;
	}

	// Last column
	src_ptr = src + (cols << 1) - 1;
	grad_ptr = grad + (cols << 1) - 1;
	angle_ptr = angle + (cols << 1) - 1;
	for(uint8_t i = 1; i < rows - 1; i++) {
		*grad_ptr = 0;
		*angle_ptr = 0;
		src_ptr += cols;
		grad_ptr += cols;
		angle_ptr += cols;
	}

	// Last row
	src_ptr = src + rows * (cols - 1) + 1;
	grad_ptr = grad + rows * (cols - 1) + 1;
	angle_ptr = angle + rows * (cols - 1) + 1;
	for(uint8_t i = 1; i < cols - 1; i++) {
		*grad_ptr = 0;
		*angle_ptr = 0;
		src_ptr++;
		grad_ptr++;
		angle_ptr++;
	}

	src_ptr = src + cols + 1;
	grad_ptr = grad + cols + 1;
	angle_ptr = angle + cols + 1;
	for(uint8_t i = 1; i < rows - 1; i++) {
		for(uint8_t j = 1; j < cols - 1; j++) {
			uint32_t gx = (uint32_t)*(src_ptr + 1) - (uint32_t)*(src_ptr - 1);
			uint32_t gy = (uint32_t)*(src_ptr + cols) - (uint32_t)*(src_ptr - cols); 
			uint32_t tmp = gx * gx + gy * gy;
			*grad_ptr = i_sqrt(tmp);
#if 1
			*angle_ptr = atan_lookup(F_DIV(gy << F_N, gx << F_N));
#else
			*angle_ptr = atan_lookup_interp(F_DIV(gy << F_N, gx << F_N));
#endif
			grad_ptr++;
			src_ptr++;
			angle_ptr++;
		}
		grad_ptr += 2;
		src_ptr += 2;
		angle_ptr += 2;
	}
	PRINTF("\r\nSobel G:\r\n");
	dump((fixed *)grad, rows, cols);
	PRINTF("\r\nSobel Theta:\r\n");
	dump(angle, rows, cols);
}


void histogram(uint16_t *grad, fixed *angle, fixed *hist, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets) {
	fixed *hist_ptr = hist;
	for(uint8_t i = 0; i < rows; i += frows) {
		for(uint8_t j = 0; j < cols; j += fcols) {
			uint16_t *grad_ptr = grad + i * cols + j;
			fixed *angle_ptr = angle + i * cols + j;
			for(uint8_t k = 0; k < buckets; k++) *(hist_ptr + k) = 0;
			for(uint8_t k = 0; k < frows; k++) {
				for(uint8_t l = 0; l < fcols; l++) {
#if 1
					*(hist_ptr + *angle_ptr) += *grad_ptr;
#else
					uint16_t idx = *angle_ptr >> F_N;
					fixed frac = F_SUB(*angle_ptr, (idx << F_N));
					*(hist_ptr + idx % buckets) += F_MUL(frac, *grad_ptr);
					*(hist_ptr + (idx + 1) % buckets) += F_MUL(
						F_SUB(F_LIT(1), frac), *grad_ptr); 
#endif

					grad_ptr++;
					angle_ptr++;
				}
				grad_ptr += cols - fcols;
				angle_ptr += cols - fcols;
			}
			hist_ptr += buckets;
		}
	}
	PRINTF("\r\nHIST:\r\n");
	dump(hist, rows, cols);
}

int32_t f_long_div(int32_t a, int32_t b) {
	int64_t tmp = a << F_N;
	if((tmp >= 0 && b >= 0) || (tmp < 0 && b < 0)) {
		tmp += b / 2;
	} else {
		tmp -= b / 2;
	}
	return (int32_t)(tmp / b);
}

void normalize(fixed *src, fixed *dest, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets) {
	fixed *dest_ptr = dest;
	uint8_t size = buckets * frows * fcols;
	uint16_t colsize = size / frows;
	uint16_t remaindersize = buckets * (cols + 1) - colsize; 
	for(uint8_t i = 0; i < rows; i++) {
		for(uint8_t j = 0; j < cols; j++) {
			fixed *src_base_ptr = src + (i * (cols + 1) + j) * buckets;
			fixed *src_ptr = src_base_ptr;
			uint32_t denom = 0;
			for(uint8_t k = 0; k < size; k++) {
				uint32_t s = *src_ptr;
				denom += s * s;
				if(k == colsize) src_ptr += remaindersize;
				else src_ptr++;
			}
			src_ptr = src_base_ptr;
			int32_t sqrt_denom = i_sqrt(denom) << F_N;
			for(uint8_t k = 0; k < size; k++) {
				int32_t tmp = ((int32_t)*src_ptr) << F_N;
				*dest_ptr++ = f_long_div(tmp, sqrt_denom);
				if(k == colsize) src_ptr += remaindersize;
				else src_ptr++;
			}
		}
	}
	PRINTF("\r\nNORM:\r\n");
	dump(dest, rows, cols);
}

fixed infer(fixed *src){
	fixed result = 0;
	for(uint16_t i = 0; i < 9576; i++) {
		result = F_ADD(result, F_MUL(histnorm[i], svm_w_0[i]));
	}
	result = F_ADD(result, svm_b_0);

	return result;
}

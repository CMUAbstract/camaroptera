#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <msp430.h>
#include "hog-svm.h"
#include "svm.h"
#include "input.h"

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
	normalize(hist, histnorm, 15, 20, 2, 2, 9);
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

#define SHORT_MAX (1 << 15)

void dump(uint16_t *src, uint8_t rows, uint8_t cols) {
	for(uint8_t i = 0; i < 16; i++) {
		for(uint8_t j = 0; j < 16; j++) {
			PRINTF("%u ", *src++);	
		}
		src = src - 16 + cols;
		PRINTF("\r\n");
	}
}

void fixed_dump(fixed *src, uint8_t rows, uint8_t cols) {
	for(uint8_t i = 0; i < 16; i++) {
		for(uint8_t j = 0; j < 16; j++) {
			PRINTF("%i ", *src++);	
		}
		src = src - 16 + cols;
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

fixed diamond_angle(fixed y, fixed x) {
	if(y >= 0) {
		if(x >= 0) return F_DIV(y, F_ADD(x, y));
		else return  F_DIV(F_SUB(F_LIT(1), x), F_SUB(y, x));
	} else{
		if(x < 0) return F_DIV(F_SUB(y, F_LIT(2)), F_ADD(x, y));
		else return F_DIV(F_ADD(F_LIT(3), x), F_SUB(x, y));
	}
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

void sobel(fixed *src, uint16_t *grad, uint16_t *angle, uint8_t rows, uint8_t cols) {
	// First row
	fixed *src_ptr = src + 1;
	uint16_t *grad_ptr = grad + 1;
	uint16_t *angle_ptr = angle + 1;
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
#if 0
			fixed sqrt_floor = F_SQRT(tmp);
			*grad_ptr = sqrt_floor;
			if(*grad_ptr < 0) *grad_ptr = F_MAX; // CLAMP
			*angle_ptr = diamond_angle(gy, gx);
#else
			*grad_ptr = i_sqrt(tmp);
			*angle_ptr = atan_lookup(F_DIV(gy << F_N, gx << F_N));
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
	dump(grad, rows, cols);
	PRINTF("\r\nSobel Theta:\r\n");
	dump(angle, rows, cols);
}


void histogram(uint16_t *grad, uint16_t *angle, uint16_t *hist, 
	uint8_t rows, uint8_t cols, uint8_t frows, uint8_t fcols, uint8_t buckets) {
	uint16_t *hist_ptr = hist;
#if 0
	fixed bucket = F_DIV(F_LIT(4), buckets);
#endif
	for(uint8_t i = 0; i < rows; i += frows) {
		for(uint8_t j = 0; j < cols; j += fcols) {
			uint16_t *grad_ptr = grad + i * cols + j;
			uint16_t *angle_ptr = angle + i * cols + j;
			for(uint8_t k = 0; k < buckets; k++) *(hist_ptr + k) = 0;
			for(uint8_t k = 0; k < frows; k++) {
				for(uint8_t l = 0; l < fcols; l++) {
#if 0
					// Weighted sum
					fixed h = *grad_ptr;
					fixed angle = F_DIV(*angle_ptr, bucket) >> F_N;
					fixed remainder = F_SUB(*angle_ptr, F_MUL(angle << F_N, bucket));
					fixed a = F_DIV(remainder, bucket);
					fixed b = F_SUB(F_LIT(1), a);
					uint16_t *tmp = hist_ptr + angle % buckets;
					*tmp = F_ADD(*tmp, F_MUL(a, h));
					tmp = hist_ptr + (angle + 1) % buckets;
					*tmp = F_ADD(*tmp, F_MUL(b, h));
#else
					*(hist_ptr + *angle_ptr) += *grad_ptr;
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

void normalize(uint16_t *src, fixed *dest, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets) {
	for(uint8_t i = 0; i < rows; i++) {
		for(uint8_t j = 0; j < cols; j++) {
			uint32_t denom = 0;
			for(uint8_t k = 0; k < frows; k++) {
				for(uint8_t l = 0; l < fcols; l++) {
					for(uint8_t m = 0; m < buckets; m++) {
						denom += *src * *src; // HERE
					}
				}
			}
			denom = i_sqrt(denom) << F_N;
			for(uint8_t k = 0; k < frows; k++) {
				for(uint8_t l = 0; l < fcols; l++) {
					for(uint8_t m = 0; m < buckets; m++) {
						fixed tmp = *src << F_N:
						*dest = F_DIV(tmp, denom); // HERE
					}
				}
			}
		}
	}
	PRINTF("\r\nNORM:\r\n");
	fixed_dump(histnorm, rows, cols);
}

fixed infer(fixed *src){
	fixed result = 0;
	for(uint16_t i = 0; i < 9576; i++) {
		result = F_ADD(result, F_MUL(histnorm[i], svm_w_0[i]));
	}
	result = F_ADD(result, svm_b_0);

	return result;
}

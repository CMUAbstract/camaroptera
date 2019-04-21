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

void dump(fixed *src, uint8_t rows, uint8_t cols, uint8_t yblock, uint8_t xblock) {
	for(uint8_t i = 0; i < yblock; i++) {
		for(uint8_t j = 0; j < xblock; j++) {
			PRINTF("%i ", *src++);	
		}
		src = src - xblock + cols;
		PRINTF("\r\n");
	}
}

uint32_t i_sqrt(uint32_t x) {
	if (x == 0 || x == 1) return x; 
	uint32_t i = 1, result = 1; 
	while(result <= x) { 
		i++; 
		result = i * i; 
	}
	return i - 1; 
}

fixed f_abs(fixed v) {
	if(v < 0) return F_MUL(F_LIT(-1), v);
	return v;
}

int32_t f_long_mul(int32_t a, int32_t b) {
	int64_t tmp = a * b;
	tmp += F_K;
	tmp >>= F_N;
	return (int32_t)tmp;
}
int32_t f_long_abs(int32_t v) {
	if(v < 0) {
		int32_t neg_one = 1 << F_N;
		neg_one *= -1;
		return f_long_mul(neg_one, v);
	}
	return v;
}

uint8_t atan_lookup(fixed a, fixed b) {
	if(F_MUL(f_abs(b) + 1, F_LIT(5.67)) < f_abs(a) || b == F_LIT(0)) return 4;

	fixed x = F_DIV(a, b + 1);
	if(x >= F_LIT(0.0) && x < F_LIT(0.36)) return 0;
	else if(x >= F_LIT(0.36) && x < F_LIT(0.84)) return 1;
	else if(x >= F_LIT(0.84) && x < F_LIT(1.73)) return 2;
	else if(x >= F_LIT(1.73) && x < F_LIT(5.67)) return 3;
	else if(x >= F_LIT(-5.67) && x < F_LIT(-1.73)) return 5;
	else if(x >= F_LIT(-1.73) && x < F_LIT(-0.84)) return 6;
	else if(x >= F_LIT(-0.84) && x < F_LIT(-0.36)) return 7;
	else if(x >= F_LIT(-0.36) && x < F_LIT(0)) return 8;
	return 4;
}

__nvram uint8_t atan_bucket_reg_pos[33] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
__nvram uint8_t atan_bucket_reg_neg[33] = {0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6};
__nvram uint8_t atan_bucket_inv_pos[33] = {4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
__nvram uint8_t atan_bucket_inv_neg[33] = {4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

int32_t f_long_div(int32_t a, int32_t b) {
	int64_t tmp = a << F_N;
	if((tmp >= 0 && b >= 0) || (tmp < 0 && b < 0)) {
		tmp += b / 2;
	} else {
		tmp -= b / 2;
	}
	return (int32_t)(tmp / b);
}

uint8_t f_atan2(int32_t y, int32_t x) {
	int32_t ax = f_long_abs(x);
	int32_t ay = f_long_abs(y); 
	if(ax == 0) return 4;
	if((y < 0 && x > 0) || (x < 0 && y > 0)) {
		if(ay > ax) {
			return atan_bucket_inv_neg[f_long_div(ax, ay)];
		}
		return atan_bucket_reg_neg[f_long_div(ay, ax)];
	}
	if(ay > ax) {
		return atan_bucket_inv_pos[f_long_div(ax, ay)];
	}
	return atan_bucket_reg_pos[f_long_div(ay, ax)];
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
#if 0
			*angle_ptr = atan_lookup(gy << F_N, gx << F_N);
#else
			*angle_ptr = f_atan2(gy << F_N, gx << F_N);
#endif
			grad_ptr++;
			src_ptr++;
			angle_ptr++;
		}
		grad_ptr += 2;
		src_ptr += 2;
		angle_ptr += 2;
	}
#ifdef DEBUG
	PRINTF("\r\nSobel G:\r\n");
	dump((fixed *)grad, rows, cols, 16, 16);
	PRINTF("\r\nSobel Theta:\r\n");
	dump(angle, rows, cols, 16, 16);
#endif
}


void histogram(uint16_t *grad, fixed *angle, uint16_t *hist, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets) {
	fixed *hist_ptr = hist;
	for(uint8_t i = 0; i < rows; i += frows) {
		for(uint8_t j = 0; j < cols; j += fcols) {
			uint16_t *grad_ptr = grad + i * cols + j;
			fixed *angle_ptr = angle + i * cols + j;
			for(uint8_t k = 0; k < buckets; k++) *(hist_ptr + k) = 0;
			for(uint8_t k = 0; k < frows; k++) {
				for(uint8_t l = 0; l < fcols; l++) {
					*(hist_ptr + *angle_ptr) += *grad_ptr;
					grad_ptr++;
					angle_ptr++;
				}
				grad_ptr += cols - fcols;
				angle_ptr += cols - fcols;
			}
			hist_ptr += buckets;
		}
	}
#ifdef DEBUG
	PRINTF("\r\nHIST:\r\n");
	dump(hist, 15, 180, 15, 18);
#endif
}

void normalize(uint16_t *src, fixed *dest, uint8_t rows, uint8_t cols, 
	uint8_t frows, uint8_t fcols, uint8_t buckets) {
	fixed *dest_ptr = dest;
	uint8_t size = buckets * frows * fcols;
	uint16_t colsize = size / frows;
	uint16_t remaindersize = buckets * (cols + 1) - colsize; 
	for(uint8_t i = 0; i < rows; i++) {
		for(uint8_t j = 0; j < cols; j++) {
			uint16_t *src_base_ptr = src + (i * (cols + 1) + j) * buckets;
			uint16_t *src_ptr = src_base_ptr;
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
#ifdef DEBUG
	PRINTF("\r\nNORM:\r\n");
	dump(dest, 14, 171, 14, 18);
#endif
}

fixed infer(fixed *src){
	fixed result = 0;
	for(uint16_t i = 0; i < 9576; i++) {
		result = F_ADD(result, F_MUL(histnorm[i], svm_w_0[i]));
	}
	result = F_ADD(result, svm_b_0);

	return result;
}

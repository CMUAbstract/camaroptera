#pragma once
#include <stdbool.h>
#include <libmat/mat.h>

#define BUFFER_NUM 2
#define BUFFER_SIZE 0x4b00

#define __fram __attribute__((section(".persistent")))
#define __ro_fram __attribute__((section(".rodata")))
#define __hifram __attribute__((section(".upper.persistent")))
#define __ro_hifram __attribute__((section(".upper.rodata")))
#define __known __attribute__((section(".known")))

extern fixed inference_buffer[BUFFER_NUM][BUFFER_SIZE];

void zero(mat_t *buffer);
void normalize(mat_t *src, mat_t *dest, int32_t max, int32_t threshold);
void relu(mat_t *src_buffer, int16_t threshold);
void pooling(mat_t *src_buffer, mat_t *dest_buffer, 
	uint8_t type, uint8_t kernel_size, uint8_t stride);
void conv_dense(mat_t *weight, mat_t *bias, mat_t *src, mat_t *dest, 
	uint16_t stride, uint8_t shift);
void fc_dense(mat_t *weight, mat_t *bias, mat_t *src, mat_t *dest, uint8_t shift);
void fc_sparse(mat_t *weight, mat_t *bias, mat_t *src, mat_t *dest, uint8_t shift);


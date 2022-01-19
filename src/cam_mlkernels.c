#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <libio/console.h>

#include <libmat/mat.h>

#include "cam_mlkernels.h"

__ro_hifram int16_t inference_buffer[BUFFER_NUM][BUFFER_SIZE];

void zero(mat_t *src) {
	uint16_t size = MAT_GET_SIZE(src);
	int16_t *data = src->data;
	for(uint16_t i = 0; i < size; i++) *data++ = 0;
}

void normalize(mat_t *src, mat_t *dest, int32_t max, int32_t threshold) {
	int32_t temp;
	uint16_t size = MAT_GET_SIZE(src);	
	int16_t *src_ptr = src->data;
	int16_t *dest_ptr = dest->data;

	for(uint16_t i = 0; i < size; i++) {
		temp = (*src_ptr++ * max) / threshold;
		*dest_ptr++ = temp;
	}
}

void relu(mat_t *src, int16_t threshold) {
	uint16_t size = MAT_GET_SIZE(src);
	int16_t *data = src->data;
	for(uint16_t i = 0; i < size; i++) {
		if(*data < threshold) *data = 0;
		++data;
	}
}

void pooling(mat_t *src, mat_t *dest, 
	uint8_t type, uint8_t kernel_size, uint8_t stride) {
	
	uint16_t channels = MAT_GET_DIM(src, 0);
	uint16_t rows = MAT_GET_DIM(dest, 1);
	uint16_t cols = MAT_GET_DIM(dest, 2);
	uint16_t stride_rows = stride * MAT_GET_STRIDE(src, 1);
	uint16_t stride_cols = stride * MAT_GET_STRIDE(src, 2);
	
	int16_t denom = 1;
	if(type) denom = kernel_size * kernel_size;

	int16_t *src_channel_ptr = src->data;
	int16_t *dest_channel_ptr = dest->data;
	for(uint16_t c = 0; c < channels; c++) {
		int16_t *src_row_ptr = src_channel_ptr;
		int16_t *dest_row_ptr = dest_channel_ptr;

		for(uint16_t i = 0; i < rows; i++) {
			int16_t *src_col_ptr = src_row_ptr;
			int16_t *dest_col_ptr = dest_row_ptr;

			for(uint16_t j = 0; j < cols; j++) {
				int32_t w = 0;
				if(type) w = SHRT_MIN;
				
				int16_t *src_kernel_row_ptr = src_col_ptr;

				for(uint16_t fi = 0; fi < kernel_size; fi++) {
					int16_t *src_kernel_col_ptr = src_kernel_row_ptr;

					for(uint16_t fj = 0; fj < kernel_size; fj++) {
						int16_t f = *src_kernel_col_ptr;
						if(type) {
							w += f;
						} else if(f > w) {
							w = f;
						}
						
						src_kernel_col_ptr += MAT_GET_STRIDE(src, 2);
					}

					src_kernel_row_ptr += MAT_GET_STRIDE(src, 1);
				}

				*dest_col_ptr = w / denom;

				src_col_ptr += stride_cols;
				dest_col_ptr += MAT_GET_STRIDE(dest, 2);
			}

			src_row_ptr += stride_rows;
			dest_row_ptr += MAT_GET_STRIDE(dest, 1);
		}

		src_channel_ptr += MAT_GET_STRIDE(src, 0);
		dest_channel_ptr += MAT_GET_STRIDE(dest, 0);
	}	
}

void conv_dense(mat_t *weight, mat_t *bias, mat_t *src, mat_t *dest, 
	uint16_t stride, uint8_t shift) {

	uint16_t output_channels = MAT_GET_DIM(weight, 0);
	uint16_t input_channels = MAT_GET_DIM(weight, 1);
	uint16_t filter_rows = MAT_GET_DIM(weight, 2);
	uint16_t filter_cols = MAT_GET_DIM(weight, 3);
	uint16_t input_rows = MAT_GET_DIM(src, 1);
	uint16_t input_cols = MAT_GET_DIM(src, 2);
	uint16_t output_rows = MAT_GET_DIM(dest, 1);
	uint16_t output_cols = MAT_GET_DIM(dest, 2);
	uint16_t stride_row = stride * MAT_GET_STRIDE(src, 1);
	uint16_t stride_col = stride * MAT_GET_STRIDE(src, 2);

	int16_t *dest_channel_ptr = dest->data;
	int16_t *filter_output_channel_ptr = weight->data;
	for(uint16_t oc = 0; oc < output_channels; oc++) {
		int16_t *src_row_ptr = src->data;
		int16_t *dest_row_ptr = dest_channel_ptr;

		for(uint16_t i = 0; i < output_rows; i++) {
			int16_t *src_col_ptr = src_row_ptr;
			int16_t *dest_col_ptr = dest_row_ptr;

			for(uint16_t j = 0; j < output_cols; j++) {
				int16_t *src_channel_ptr = src_col_ptr;
				int16_t *filter_input_channel_ptr = filter_output_channel_ptr;
				int32_t w = 0;

				for(uint16_t fc = 0; fc < input_channels; fc++) {
					int16_t *src_filter_row_ptr = src_channel_ptr;
					int16_t *filter_row_ptr = filter_input_channel_ptr;

					for(uint16_t fi = 0; fi < filter_rows; fi++) {
						int16_t *src_filter_col_ptr = src_filter_row_ptr;
						int16_t *filter_col_ptr = filter_row_ptr;

						for(uint16_t fj = 0; fj < filter_cols; fj++) {
							w += *filter_col_ptr * *src_filter_col_ptr;

							src_filter_col_ptr += MAT_GET_STRIDE(src, 2);
							filter_col_ptr += MAT_GET_STRIDE(weight, 3);
						}

						src_filter_row_ptr += MAT_GET_STRIDE(src, 1);
						filter_row_ptr += MAT_GET_STRIDE(weight, 2);
					}

					src_channel_ptr += MAT_GET_STRIDE(src, 0);
					filter_input_channel_ptr += MAT_GET_STRIDE(weight, 1);
				}

				w >>= shift;
				if(w < SHRT_MIN) w = SHRT_MIN;
				if(w > SHRT_MAX) w = SHRT_MAX;
				*dest_col_ptr = w;

				src_col_ptr += stride_col;
				dest_col_ptr += MAT_GET_STRIDE(dest, 2);
			}

			src_row_ptr += stride_row;
			dest_row_ptr += MAT_GET_STRIDE(dest, 1);
		}

		dest_channel_ptr += MAT_GET_STRIDE(dest, 0);
		filter_output_channel_ptr += MAT_GET_STRIDE(weight, 0);
	}

	if(bias == NULL) return;

	int16_t *bias_ptr = bias->data;
	dest_channel_ptr = dest->data;
	for(uint16_t oc = 0; oc < output_channels; oc++) {
		int16_t *dest_row_ptr = dest_channel_ptr;

		for(uint16_t i = 0; i < output_rows; i++) {
			int16_t *dest_col_ptr = dest_row_ptr;

			for(uint16_t j = 0; j < output_cols; j++) {
				*dest_col_ptr += *bias_ptr;

				dest_col_ptr += MAT_GET_STRIDE(dest, 2);
			}

			dest_row_ptr += MAT_GET_STRIDE(dest, 1);
		}

		dest_channel_ptr += MAT_GET_STRIDE(dest, 0);
		bias_ptr += MAT_GET_STRIDE(bias, 0);
	}
}

void fc_dense(mat_t *weight, mat_t *bias, mat_t *src, mat_t *dest, uint8_t shift) {
	uint16_t rows = MAT_GET_DIM(dest, 0);
	uint16_t cols = MAT_GET_DIM(weight, 1);

	int16_t *filter_row_ptr = weight->data;
	int16_t *dest_ptr = dest->data;
	for(uint16_t i = 0; i < rows; i++) {
		int32_t w = 0;
		int16_t *src_ptr = src->data;
		int16_t *filter_col_ptr = filter_row_ptr;
		for(uint16_t j = 0; j < cols; j++) {
			w += *src_ptr * *filter_col_ptr;

			src_ptr++;
			filter_col_ptr += MAT_GET_STRIDE(weight, 1);
		}

		w >>= shift;
		if(w < SHRT_MIN) w = SHRT_MIN;
		if(w > SHRT_MAX) w = SHRT_MAX;
		*dest_ptr++ = w;

		filter_row_ptr += MAT_GET_STRIDE(weight, 0);
	}

	if(bias == NULL) return;

	int16_t *bias_ptr = bias->data;
	dest_ptr = dest->data;
	for(uint16_t i = 0; i < rows; ++i) {
		*dest_ptr += *bias_ptr;

		dest_ptr += MAT_GET_STRIDE(dest, 0);
		bias_ptr += MAT_GET_STRIDE(bias, 0);
	}
}

void fc_sparse(mat_t *weight, mat_t *bias, mat_t *src, mat_t *dest, uint8_t shift) {
	uint16_t rows = MAT_GET_DIM(dest, 0);

	int16_t *dest_ptr = dest->data;
	for(uint16_t i = 0; i < rows; i++) {
		int32_t w = 0;
		uint16_t start = weight->sparse.sizes[i];
		uint16_t end = weight->sparse.sizes[i + 1];
		uint16_t *offset_ptr = weight->sparse.offsets + start;
		int16_t *filter_ptr = weight->data + start;

		for(uint16_t j = start; j < end; j++) {
			w += *(src->data + *offset_ptr) * *filter_ptr;
			filter_ptr++;
			offset_ptr++;
		}

		w >>= shift;
		if(w < SHRT_MIN) w = SHRT_MIN;
		if(w > SHRT_MAX) w = SHRT_MAX;
		*dest_ptr++ = w;
	}

	if(bias == NULL) return;

	int16_t *bias_ptr = bias->data;
	dest_ptr = dest->data;
	for(uint16_t i = 0; i < rows; ++i) {
		*dest_ptr += *bias_ptr;

		dest_ptr += MAT_GET_STRIDE(dest, 0);
		bias_ptr += MAT_GET_STRIDE(bias, 0);
	}
}
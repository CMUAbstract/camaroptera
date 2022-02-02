#include <stdint.h>

#include <libfixed/fixed.h>
#include <libmat/mat.h>
#include <stddef.h>
#include <stdbool.h>

#include "dnn_layers.h"
#include "cam_mlkernels.h"

__fram uint8_t *input_ptr = NULL;
__fram mat_t buf1 = {.data = inference_buffer[0]};
__fram mat_t buf2 = {.data = inference_buffer[1]};
__fram mat_t *b1 = &buf1;
__fram mat_t *b2 = &buf2;

void dnn_init(uint8_t *data){
	input_ptr = data; 
}

uint8_t dnn_get_class_result() {
	fixed max = MAT_GET(b1, 0, 0);;
	uint8_t predict = 0;
	for(uint16_t i = 0; i < CLASSES; i++) {
		fixed v = MAT_GET(b2, i, 0);
		if(v > max) {
			predict = i;
			max = v;
		}
	}
	return predict;
}

#include "headers/conv_1_weight.h"
#include "headers/conv_2_weight.h"
#include "headers/conv_3_weight.h"
#include "headers/conv_6_weight.h"
#include "headers/conv_7_weight.h"
#include "headers/conv_8_weight.h"
#include "headers/fc_0_weight.h"
#include "headers/fc_0_bias.h"
#include "headers/fc_2_weight.h"
#include "headers/fc_2_bias.h"

__ro_fram mat_t mat_conv_1_weight = {
	.dims = {1, 1, 3, 1},
	.strides = {3, 3, 1, 1},
	.len_dims = 4,
	.data = conv_1_weight
};

__ro_fram mat_t mat_conv_2_weight = {
	.dims = {1, 1, 1, 3},
	.strides = {3, 3, 3, 1},
	.len_dims = 4,
	.data = conv_2_weight
};

__ro_fram mat_t mat_conv_3_weight = {
	.dims = {8, 1, 1, 1},
	.strides = {1, 1, 1, 1},
	.len_dims = 4,
	.data = conv_3_weight
};

__ro_fram mat_t mat_conv_6_weight = {
	.dims = {8, 8, 3, 1},
	.strides = {24, 3, 1, 1},
	.len_dims = 4,
	.data = conv_6_weight
};

__ro_fram mat_t mat_conv_7_weight = {
	.dims = {8, 8, 1, 3},
	.strides = {24, 3, 3, 1},
	.len_dims = 4,
	.data = conv_7_weight
};

__ro_fram mat_t mat_conv_8_weight = {
	.dims = {8, 8, 1, 1},
	.strides = {8, 1, 1, 1},
	.len_dims = 4,
	.data = conv_8_weight
};

__ro_fram mat_t mat_fc_0_weight = {
	.dims = {64, 384},
	.strides = {384, 1},
	.len_dims = 2,
	.data = fc_0_weight
};

__ro_fram mat_t mat_fc_0_bias = {
	.dims = {64, 1},
	.strides = {1, 1},
	.len_dims = 1,
	.data = fc_0_bias
};

__ro_fram mat_t mat_fc_2_weight = {
	.dims = {2, 64},
	.strides = {64, 1},
	.len_dims = 2,
	.data = fc_2_weight
};

__ro_fram mat_t mat_fc_2_bias = {
	.dims = {2, 1},
	.strides = {1, 1},
	.len_dims = 1,
	.data = fc_2_bias
};

void dnn_L0_pool() {
	MAT_RESHAPE(b1, 1, 120, 160);

	uint16_t size = MAT_GET_SIZE(b1);
	uint8_t *src_ptr = input_ptr;
	int16_t *dest_ptr = b1->data;
	for(uint16_t i = 0; i < size; i++) {
		*dest_ptr++ = *src_ptr++;
	}	

	normalize(b1, b1, 256, 255);

	MAT_RESHAPE(b2, 1, 30, 40);
	pooling(b1, b2, 0, 4, 4);
}

void dnn_L1_conv() {
	MAT_RESHAPE(b1, 1, 28, 40);
	mat_t *w_ptr = &mat_conv_1_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L2_conv() {
	MAT_RESHAPE(b2, 1, 28, 38);
	mat_t *w_ptr = &mat_conv_2_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b1, b2, 1, 8);
}

void dnn_L3_conv() {
	MAT_RESHAPE(b1, 8, 28, 38);
	mat_t *w_ptr = &mat_conv_3_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L4_relu() {
	MAT_RESHAPE(b1, 8, 28, 38);
	relu(b1, 0);
}

void dnn_L5_pool() {
	MAT_RESHAPE(b2, 8, 14, 19);
	pooling(b1, b2, 0, 2, 2);
}

void dnn_L6_conv() {
	MAT_RESHAPE(b1, 8, 12, 19);
	mat_t *w_ptr = &mat_conv_6_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L7_conv() {
	MAT_RESHAPE(b2, 8, 12, 17);
	mat_t *w_ptr = &mat_conv_7_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b1, b2, 1, 8);
}

void dnn_L8_conv() {
	MAT_RESHAPE(b1, 8, 12, 17);
	mat_t *w_ptr = &mat_conv_8_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L9_relu() {
	MAT_RESHAPE(b1, 8, 12, 17);
	relu(b1, 0);
}

void dnn_L10_pool() {
	MAT_RESHAPE(b2, 8, 6, 8);
	pooling(b1, b2, 0, 2, 2);
}

void dnn_L12_fc() {
	MAT_RESHAPE(b2, 384, 1);
	MAT_RESHAPE(b1, 64, 1);
	mat_t *w_ptr = &mat_fc_0_weight;
	mat_t *b_ptr = &mat_fc_0_bias;
	fc_dense(w_ptr, b_ptr, b2, b1, 8);
}

void dnn_L13_relu() {
	MAT_RESHAPE(b1, 64, 1);
	relu(b1, 0);
}

void dnn_L14_fc() {
	MAT_RESHAPE(b2, 2, 1);
	mat_t *w_ptr = &mat_fc_2_weight;
	mat_t *b_ptr = &mat_fc_2_bias;
	fc_dense(w_ptr, b_ptr, b1, b2, 8);
}
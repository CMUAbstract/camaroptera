#include <stdint.h>

#include <libfixed/fixed.h>
#include <libmat/mat.h>
#include <stddef.h>
#include <stdbool.h>

#include "dnn_layers.h"
#include "cam_mlkernels.h"

__ro_fram mat_t mat_input = {
	.dims = {1, 120, 160},
	.strides = {19200, 160, 1},
	.len_dims = 3,
	.data = NULL,
};

__fram mat_t buf1 = {.data = inference_buffer[0]};
__fram mat_t buf2 = {.data = inference_buffer[1]};
__fram mat_t *b1 = &buf1;
__fram mat_t *b2 = &buf2;

void dnn_init(void *data){
	mat_input.data = (fixed *)data; 
}

uint8_t dnn_get_class_result() {
	fixed max = 0;
	uint8_t predict = 0;
	for(uint16_t i = 0; i < CLASSES; i++) {
		fixed v = MAT_GET(b1, i, 0, 0);

		if(v > max) {
			predict = i;
			max = v;
		}
	}
	return predict;
}

#include "headers/conv_1_weight.h"
#include "headers/conv_4_weight.h"
#include "headers/conv_5_weight.h"
#include "headers/conv_6_weight.h"
#include "headers/conv_9_weight.h"
#include "headers/conv_10_weight.h"
#include "headers/conv_11_weight.h"
#include "headers/fc_0_weight.h"
#include "headers/fc_0_bias.h"
#include "headers/fc_2_weight.h"
#include "headers/fc_2_bias.h"

__ro_fram mat_t mat_conv_1_weight = {
	.dims = {4, 1, 1, 1},
	.strides = {1, 1, 1, 1},
	.len_dims = 4,
	.data = conv_1_weight
};

__ro_fram mat_t mat_conv_4_weight = {
	.dims = {4, 4, 3, 1},
	.strides = {12, 3, 1, 1},
	.len_dims = 4,
	.data = conv_4_weight
};

__ro_fram mat_t mat_conv_5_weight = {
	.dims = {4, 4, 1, 3},
	.strides = {12, 3, 3, 1},
	.len_dims = 4,
	.data = conv_5_weight
};

__ro_fram mat_t mat_conv_6_weight = {
	.dims = {16, 4, 1, 1},
	.strides = {4, 1, 1, 1},
	.len_dims = 4,
	.data = conv_6_weight
};

__ro_fram mat_t mat_conv_9_weight = {
	.dims = {16, 16, 3, 1},
	.strides = {48, 3, 1, 1},
	.len_dims = 4,
	.data = conv_9_weight
};

__ro_fram mat_t mat_conv_10_weight = {
	.dims = {16, 16, 1, 3},
	.strides = {48, 3, 3, 1},
	.len_dims = 4,
	.data = conv_10_weight
};

__ro_fram mat_t mat_conv_11_weight = {
	.dims = {16, 16, 1, 1},
	.strides = {16, 1, 1, 1},
	.len_dims = 4,
	.data = conv_11_weight
};

__ro_fram mat_t mat_fc_0_weight = {
	.dims = {FC_0_WEIGHT_SIZE},
	.strides = 1,
	.len_dims = 1,
	.data = fc_0_weight,
	.sparse = {
		.dims = {64, 3264},
		.len_dims = 2,
		.sizes = fc_0_weight_indptr,
		.offsets = fc_0_weight_index
	}};

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
	mat_t *mat_input_ptr = &mat_input;
	normalize(mat_input_ptr, b1, 256, 255);

	MAT_RESHAPE(b2, 1, 60, 80);
	pooling(b1, b2, 0, 2, 2);
}

void dnn_L1_conv() {
	MAT_RESHAPE(b1, 4, 60, 80);
	mat_t *w_ptr = &mat_conv_1_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L2_relu() {
	MAT_RESHAPE(b1, 4, 60, 80);
	relu(b1, 0);
}

void dnn_L3_pool() {
	MAT_RESHAPE(b2, 4, 30, 40);
	pooling(b1, b2, 0, 2, 2);
}

void dnn_L4_conv() {
	MAT_RESHAPE(b1, 4, 28, 40);
	mat_t *w_ptr = &mat_conv_4_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L5_conv() {
	MAT_RESHAPE(b2, 4, 28, 38);
	mat_t *w_ptr = &mat_conv_5_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b1, b2, 1, 8);
}

void dnn_L6_conv() {
	MAT_RESHAPE(b1, 16, 28, 38);
	mat_t *w_ptr = &mat_conv_6_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L7_relu() {
	MAT_RESHAPE(b1, 16, 28, 38);
	relu(b1, 0);
}

void dnn_L8_pool() {
	MAT_RESHAPE(b2, 16, 14, 19);
	pooling(b1, b2, 0, 2, 2);
}

void dnn_L9_conv() {
	MAT_RESHAPE(b1, 16, 12, 19);
	mat_t *w_ptr = &mat_conv_9_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L10_conv() {
	MAT_RESHAPE(b2, 16, 12, 17);
	mat_t *w_ptr = &mat_conv_10_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b1, b2, 1, 8);
}

void dnn_L11_conv() {
	MAT_RESHAPE(b1, 16, 12, 17);
	mat_t *w_ptr = &mat_conv_11_weight;
	mat_t *b_ptr = NULL;
	conv_dense(w_ptr, b_ptr, b2, b1, 1, 8);
}

void dnn_L12_relu() {
	MAT_RESHAPE(b1, 16, 12, 17);
	relu(b1, 0);
}

void dnn_L14_fc() {
	MAT_RESHAPE(b1, 3264, 1);
	MAT_RESHAPE(b2, 64, 1);
	mat_t *w_ptr = &mat_fc_0_weight;
	mat_t *b_ptr = &mat_fc_0_bias;
	fc_sparse(w_ptr, b_ptr, b1, b2, 8);
}

void dnn_L15_relu() {
	MAT_RESHAPE(b2, 64, 1);
	relu(b2, 0);
}

void dnn_L16_fc() {
	// MAT_RESHAPE(b2, FILL, 1);
	MAT_RESHAPE(b1, 2, 1);
	mat_t *w_ptr = &mat_fc_2_weight;
	mat_t *b_ptr = &mat_fc_2_bias;
	fc_dense(w_ptr, b_ptr, b2, b1, 8);
}


#include <stdbool.h>
#include <stddef.h>

#include "libfixed/fixed.h"
#include "libmat/mat.h"
#include "dnn.h"
#include "lenet.h"
#include "main.h"

#include "headers_30x40/conv1.h"
#include "headers_30x40/conv2.h"
#include "headers_30x40/fc1.h"
#include "headers_30x40/fc2.h"

#define BUFFER_NUM 2
#define BUFFER_SIZE 0x6200

fixed inference_buffer[BUFFER_NUM][BUFFER_SIZE];

extern uint8_t frame[];


__ro_flash mat_t mat_conv1_wd = {
	.dims = {CONV1_WMD_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_wmd,
	.sparse = {
		.dims = {20, 1, 1, 1},
		.len_dims = 4,
		.sizes = conv1_wmd_sizes,
		.offsets = conv1_wmd_offsets
	},
};

__ro_flash mat_t mat_conv1_wv = {
	.dims = {CONV1_WMV_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_wmv,
	.sparse = {
		.dims = {20, 1, 5, 1},
		.len_dims = 4,
		.sizes = conv1_wmv_sizes,
		.offsets = conv1_wmv_offsets
	},
};

__ro_flash mat_t mat_conv1_wh = {
	.dims = {CONV1_WMH_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_wmh,
	.sparse = {
		.dims = {20, 1, 1, 5},
		.len_dims = 4,
		.sizes = conv1_wmh_sizes,
		.offsets = conv1_wmh_offsets
	},
};

__ro_flash mat_t mat_conv1_b = {
	.dims = {20},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_b,
};

__ro_flash mat_t mat_conv2_wd = {
	.dims = {CONV2_WMD_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_wmd,
	.sparse = {
		.dims = {100, 20, 1, 1},
		.len_dims = 4,
		.sizes = conv2_wmd_sizes,
		.offsets = conv2_wmd_offsets
	},
};

__ro_flash mat_t mat_conv2_wv = {
	.dims = {CONV2_WMV_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_wmv,
	.sparse = {
		.dims = {100, 1, 5, 1},
		.len_dims = 4,
		.sizes = conv2_wmv_sizes,
		.offsets = conv2_wmv_offsets
	},
};

__ro_flash mat_t mat_conv2_wh = {
	.dims = {CONV2_WMH_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_wmh,
	.sparse = {
		.dims = {100, 1, 1, 5},
		.len_dims = 4,
		.sizes = conv2_wmh_sizes,
		.offsets = conv2_wmh_offsets
	},
};

__ro_flash mat_t mat_conv2_b = {
	.dims = {100},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_b,
};


__ro_flash mat_t mat_fc1_wh = {
	.dims = {FC1_WMH_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = fc1_wmh,
	.sparse = {
		.dims = {100, 1, 1, 3500},
		.len_dims = 4,
		.offsets = fc1_wmh_offsets,
		.sizes = fc1_wmh_sizes,
	},
};

__ro_flash mat_t mat_fc1_wv = {
	.dims = {FC1_WMV_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = fc1_wmv,
	.sparse = {
		.dims = {500, 1, 1, 100},
		.len_dims = 4,
		.offsets = fc1_wmv_offsets,
		.sizes = fc1_wmv_sizes,
	},
};

__ro_flash mat_t mat_fc1_b = {
	.dims = {500, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc1_b,
};

__ro_flash mat_t mat_fc2_w = {
	.dims = {2, 1, 1, 500},
	.strides = {500, 500 ,500, 1},
	.len_dims = 4,
	.data = fc2_w,
};

__ro_flash mat_t mat_fc2_b = {
	.dims = {2, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc2_b,
};

__ro_flash mat_t mat_input = {
	.dims = {1, 120, 160},
	.strides = {19200, 160, 1},
	.len_dims = 3,
	.data = frame,
};

extern uint8_t dnn_done, result;

uint16_t state = 0;

mat_t buf1 = {.data = inference_buffer[0]};
mat_t buf2 = {.data = inference_buffer[1]};
mat_t *b1 = &buf1;
mat_t *b2 = &buf2;

void run_DNN() {

	if(state == 0) {
		MAT_RESHAPE(b1, 1, 120, 160);
		mat_t *mat_input_ptr = &mat_input;
		normalize( mat_input_ptr, b1 );
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	
		MAT_RESHAPE(b2, 1, 30, 40);
		pooling( b1, b2, 1, 4, 4 ); 
		state = 1;
	} else if(state == 1) {
		MAT_RESHAPE(b1, 20, 30, 40);
		zero(b1);
		mat_t *w_ptr = &mat_conv1_wd;
		mat_t *b_ptr = NULL;
		conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, false);
		state = 2;
	}else if(state == 2) {
		MAT_RESHAPE(b2, 20, 30, 36);
		zero(b2);
		mat_t *w_ptr = &mat_conv1_wh;
		mat_t *b_ptr = NULL;
		uint16_t running_size=0;
		for(uint16_t i = 0; i < 20; i++ ){
			// PRINTF("\r\n Layer 2 : Depth %i", i);
			mat_t w_slice;

			w_slice = MAT_CONSTRAIN(w_ptr, running_size);
			w_slice.dims[0] = w_ptr->sparse.sizes[i];
			w_slice.sparse.sizes = (w_ptr->sparse.sizes + i);
			w_slice.sparse.len_dims = w_ptr->sparse.len_dims - 1;
			// PRINTF("(%i,%i,%i)\r\n", w_slice.dims[0], w_slice.len_dims, w_slice.sparse.len_dims);
			running_size += w_ptr->sparse.sizes[i];

			conv_sparse( &w_slice, b_ptr, b1, b2, 0, true, i, false );
		}

		state = 3;
	} else if(state == 3) {
		MAT_RESHAPE(b1, 20, 26, 36);
		zero(b1);
		mat_t *w_ptr = &mat_conv1_wv;
		mat_t *b_ptr = &mat_conv1_b;
		uint16_t running_size=0;
		for(uint16_t i = 0; i < 20; i++ ){
			// PRINTF("\r\n Layer 3 : Depth %i", i);
			mat_t w_slice;

			w_slice = MAT_CONSTRAIN(w_ptr, running_size);
			w_slice.dims[0] = w_ptr->sparse.sizes[i];
			w_slice.sparse.sizes = (w_ptr->sparse.sizes + i);
			w_slice.sparse.len_dims = w_ptr->sparse.len_dims - 1;
			// PRINTF("(%i,%i,%i)\r\n", w_slice.dims[0], w_slice.len_dims, w_slice.sparse.len_dims);
			running_size += w_ptr->sparse.sizes[i];
			conv_sparse( &w_slice, b_ptr, b2, b1, 0, true, i, false );
		}
		state = 4;
	} else if(state == 4) {
		MAT_RESHAPE(b2, 20, 26, 36);
		relu( b1, 0 );
		state = 5;
	} else if(state == 5) {
		MAT_RESHAPE(b2, 20, 13, 18);
		zero(b2);
		pooling( b1, b2, 0, 2, 2 );
		state = 6;
	} else if(state == 6) {

		MAT_RESHAPE(b1, 100, 13, 18);
		zero(b1);
		mat_t *w_ptr = &mat_conv2_wd;
		mat_t *b_ptr = NULL;
		conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, false);
		state = 7;
	}else if(state == 7) {
		MAT_RESHAPE(b2, 100, 13, 14);
		zero(b2);
		mat_t *w_ptr = &mat_conv2_wh;
		mat_t *b_ptr = NULL;
		uint16_t running_size=0;
		for(uint16_t i = 0; i < 100; i++ ){
			// PRINTF("\r\n Layer 7 : Depth %i", i);
			mat_t w_slice;

			w_slice = MAT_CONSTRAIN(w_ptr, running_size);
			w_slice.dims[0] = w_ptr->sparse.sizes[i];
			w_slice.sparse.sizes = (w_ptr->sparse.sizes + i);
			w_slice.sparse.len_dims = w_ptr->sparse.len_dims - 1;
			running_size += w_ptr->sparse.sizes[i];
			conv_sparse( &w_slice, b_ptr, b1, b2, 0, true, i, false );
		}
		state = 8;
	} else if(state == 8) {
		MAT_RESHAPE(b1, 100, 9, 14);
		zero(b1);
		mat_t *w_ptr = &mat_conv2_wv;
		mat_t *b_ptr = &mat_conv2_b;
		uint16_t running_size=0;
		for(uint16_t i = 0; i < 100; i++ ){
			// PRINTF("\r\n Layer 8 : Depth %i", i);
			mat_t w_slice;

			w_slice = MAT_CONSTRAIN(w_ptr, running_size);
			w_slice.dims[0] = w_ptr->sparse.sizes[i];
			w_slice.sparse.sizes = (w_ptr->sparse.sizes + i);
			w_slice.sparse.len_dims = w_ptr->sparse.len_dims - 1;
			// PRINTF("(%i,%i,%i)\r\n", w_slice.dims[0], w_slice.len_dims, w_slice.sparse.len_dims);
			running_size += w_ptr->sparse.sizes[i];
			conv_sparse( &w_slice, b_ptr, b2, b1, 0, true, i, false );
		}
		state = 9;
	} else if(state == 9) {
		relu( b1, 0 );
		state = 10;
	} else if(state == 10) {
		MAT_RESHAPE(b2, 100, 5, 7);
		pooling( b1, b2, 0, 2, 2 );
		state = 11;
	} else if(state == 11) {
		MAT_RESHAPE(b2, 1, 1, 3500);
		MAT_RESHAPE(b1, 100, 1, 1);
		zero(b1);
		mat_t *w_ptr = &mat_fc1_wh;
		mat_t *b_ptr = NULL;
		conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, true );
		state = 12;
	} else if(state == 12) {
		MAT_RESHAPE(b1, 1, 1, 100);
		MAT_RESHAPE(b2, 500, 1, 1);
		zero(b2);
		mat_t *w_ptr = &mat_fc1_wv;
		mat_t *b_ptr = &mat_fc1_b;
		conv_sparse( w_ptr, b_ptr, b1, b2, 0, false, 0, true);
		state = 13;
	} else if(state == 13) {
		MAT_RESHAPE(b2, 1, 1, 500);
		MAT_RESHAPE(b1, 500, 1);
		relu( b2, 0 );
		state = 14;
	} else if(state == 14) {
		MAT_RESHAPE(b1, 2, 1, 1);
		zero(b1);
		mat_t *w_ptr = &mat_fc2_w;
		mat_t *b_ptr = &mat_fc2_b;
		conv_dense( w_ptr, b_ptr, b2, b1, 0 );
		task_finish();
		state = 0;
	}
}

void task_finish() {
	uint8_t predict;
	fixed max = 0;
	for( uint16_t i = 0; i < 2; i++ ) {
		fixed v = MAT_GET(b1, i, 0, 0);
		if(v > max) {
			predict = i;
			max = v;
		}
	}
	result = predict;
	task_exit();
}

void task_exit() {

	dnn_done = 1;
}


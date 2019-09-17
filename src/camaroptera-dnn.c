#include <stdbool.h>
#include <msp430.h>
#include <stdlib.h>
#include <string.h>

#include <libio/console.h>
#include <libmspbuiltins/builtins.h>
#include <libmsp/mem.h>
#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <libfixed/fixed.h>
#include <libmat/mat.h>


#include "camaroptera-dnn.h"
#include "lenet.h"
#include "headers_30x40/conv1.h"
#include "headers_30x40/conv2.h"
#include "headers_30x40/fc1.h"
#include "headers_30x40/fc2.h"
#include "experiment_array.h"

extern uint8_t frame[];
extern uint8_t camaroptera_state;
extern uint8_t read_status;
extern uint16_t person_count;
extern uint16_t not_sent_count;
extern uint16_t sent_packet_count;
extern int8_t last_state;


__ro_hifram uint8_t index_img_seq = 0;
__ro_hifram uint8_t index_fp = 0;
__ro_hifram uint8_t index_fn = 0;

void init();
//#define PRINT_DEBUG

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////Alapaca Shim///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define MEM_SIZE 0x400
__hifram uint8_t *data_src[MEM_SIZE];
__hifram uint8_t *data_dest[MEM_SIZE];
__hifram unsigned int data_size[MEM_SIZE];
void clear_isDirty() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Tasks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TASK(1, camaroptera_main);
TASK(2, task_init);
TASK(3, task_compute);
TASK(4, task_finish);
TASK(5, task_exit);

ENTRY_TASK(camaroptera_main)
INIT_FUNC(init)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Setup///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_CONSOLE
	#pragma message "no console"
	#define printf(fmt, ...) (void)0
#endif

static void init_hw() {
	msp_watchdog_disable();
	msp_gpio_unlock();
	msp_clock_setup();
}

void init() {
	init_hw();

#ifdef CONFIG_CONSOLE
	#pragma message "init console"
	INIT_CONSOLE();
#endif

	__enable_interrupt();

	PRINTF(".%u.\r\n", curctx->task->idx);
	
	camaroptera_state = 0;
	person_count = 0;
	not_sent_count = 0;
	sent_packet_count = 0;
	index_fn = 0;
	index_fp = 0;
	last_state = 0; // EXP2
	
	/*
	// EXP1
	if(index_img_seq == 0)
		last_state = -1;
	else
		last_state = image_sequence[index_img_seq-1];
	*/

	P8OUT &= ~(BIT1+BIT2+BIT3);
	P8DIR |= (BIT1+BIT2+BIT3);

	P1OUT &= ~(BIT6+BIT7);
	P1DIR |= (BIT6+BIT7);

	P8DIR &= ~BIT3; 
	/*
	P1DIR = 0x00;
  P2DIR = 0x00;   
  P3DIR = 0x00;   
  P4DIR = 0x00;   
  P5DIR = 0x00;
  P6DIR = 0x00;
  P7DIR = 0x00;   
  P8DIR = 0x00;   
	*/
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Weights Matrices/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
__ro_fram mat_t mat_conv1_wd = {
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

__ro_fram mat_t mat_conv1_wv = {
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

__ro_fram mat_t mat_conv1_wh = {
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

__ro_fram mat_t mat_conv1_b = {
	.dims = {20},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_b,
};

__ro_fram mat_t mat_conv2_wd = {
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

__ro_fram mat_t mat_conv2_wv = {
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

__ro_fram mat_t mat_conv2_wh = {
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

__ro_fram mat_t mat_conv2_b = {
	.dims = {100},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_b,
};


__ro_fram mat_t mat_fc1_wh = {
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

__ro_fram mat_t mat_fc1_wv = {
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

__ro_fram mat_t mat_fc1_b = {
	.dims = {500, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc1_b,
};

__ro_fram mat_t mat_fc2_w = {
	.dims = {2, 1, 1, 500},
	.strides = {500, 500 ,500, 1},
	.len_dims = 4,
	.data = fc2_w,
};

__ro_fram mat_t mat_fc2_b = {
	.dims = {2, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc2_b,
};

__ro_fram mat_t mat_input = {
	.dims = {1, 120, 160},
	.strides = {19200, 160, 1},
	.len_dims = 3,
	.data = frame,
};

__fram mat_t buf1 = {.data = inference_buffer[0]};
__fram mat_t buf2 = {.data = inference_buffer[1]};
__fram mat_t *b1 = &buf1;
__fram mat_t *b2 = &buf2;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Tasks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void task_init() {

	PRINTF("\r\n========================");
	PRINTF("\r\nInit");

	TRANSITION_TO(task_compute);
}

void task_compute() {
	uint16_t state = CUR_SCRATCH[0];
	if(state == 0) {
		PRINTF("====INPUT IMAGE====\r\n");
		MAT_RESHAPE(b1, 1, 120, 160);
		mat_t *mat_input_ptr = &mat_input;
		normalize( mat_input_ptr, b1 );
		MAT_RESHAPE(b2, 1, 30, 40);
		pooling( b1, b2, 1, 4, 4 ); 
		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
#ifdef PRINT_DEBUG
		MAT_DUMP(b2, 0);
#endif
		transition_to(CUR_TASK);
	} else if(state == 1) {
		PRINTF("\r\n Layer 1 : Depthwise Conv1");

		MAT_RESHAPE(b1, 20, 30, 40);
		zero(b1);
		mat_t *w_ptr = &mat_conv1_wd;
		mat_t *b_ptr = NULL;
		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, false);
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 20; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b1, i);
			}
#endif
		transition_to(CUR_TASK);
	}else if(state == 2) {
		PRINTF("\r\n Layer 2 : Horizontal Conv1");

		MAT_RESHAPE(b2, 20, 30, 36);
		zero(b2);
		mat_t *w_ptr = &mat_conv1_wh;
		mat_t *b_ptr = NULL;
		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
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
#ifdef PRINT_DEBUG
		 for(uint16_t i = 0; i < 20; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b2, i);
			}
#endif

		transition_to(CUR_TASK);
	} else if(state == 3) {
		PRINTF("\r\n Layer 3 : Vertical Conv1");

		MAT_RESHAPE(b1, 20, 26, 36);
		zero(b1);
		mat_t *w_ptr = &mat_conv1_wv;
		mat_t *b_ptr = &mat_conv1_b;
		scratch_bak[0] = 4;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
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
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 20; i++ ){
		 		PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b1, i);
			}
#endif
		transition_to(CUR_TASK);
	} else if(state == 4) {
		PRINTF("\r\n Layer 4 : RELU after Conv1");

		MAT_RESHAPE(b2, 20, 26, 36);
		scratch_bak[0] = 5;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		relu( b1, 0 );
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 20; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b1, i);
			}
#endif
		transition_to(CUR_TASK);
	} else if(state == 5) {
		PRINTF("\r\n Layer 5 : Max Pooling after Conv1");

		MAT_RESHAPE(b2, 20, 13, 18);
		zero(b2);
		scratch_bak[0] = 6;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		pooling( b1, b2, 0, 2, 2 );
#ifdef PRINT_DEBUG
		 for(uint16_t i = 0; i < 20; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b2, i);
			}
#endif
		transition_to(CUR_TASK);
	} else if(state == 6) {
		PRINTF("\r\n Layer 6 : Depthwise Conv2");

		MAT_RESHAPE(b1, 100, 13, 18);
		zero(b1);
		mat_t *w_ptr = &mat_conv2_wd;
		mat_t *b_ptr = NULL;
		scratch_bak[0] = 7;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, false);
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 100; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b1, i);
			}
#endif
		transition_to(CUR_TASK);
	}else if(state == 7) {
		PRINTF("\r\n Layer 7 : Horizontal Conv2");

		MAT_RESHAPE(b2, 100, 13, 14);
		zero(b2);
		mat_t *w_ptr = &mat_conv2_wh;
		mat_t *b_ptr = NULL;
		scratch_bak[0] = 8;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
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
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 100; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b2, i);
			 }
#endif
		transition_to(CUR_TASK);
	} else if(state == 8) {
		PRINTF("\r\n Layer 8 : Vertical Conv2");

		MAT_RESHAPE(b1, 100, 9, 14);
		zero(b1);
		mat_t *w_ptr = &mat_conv2_wv;
		mat_t *b_ptr = &mat_conv2_b;
		scratch_bak[0] = 9;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
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
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 100; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b1, i);
			}
#endif
		transition_to(CUR_TASK);
	} else if(state == 9) {
		PRINTF("\r\n Layer 9 : RELU after Conv2");
		scratch_bak[0] = 10;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		relu( b1, 0 );
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 100; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b1, i);
			}
#endif
		transition_to(CUR_TASK);
	} else if(state == 10) {
		PRINTF("\r\n Layer 10 : Max Pooling after Conv2");

		MAT_RESHAPE(b2, 100, 5, 7);
		scratch_bak[0] = 11;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		pooling( b1, b2, 0, 2, 2 );
#ifdef PRINT_DEBUG
			for(uint16_t i = 0; i < 100; i++ ){
				PRINTF("====FILTER %i====\r\n", i);
				MAT_DUMP(b2, i);
			}
#endif
		transition_to(CUR_TASK);
	} else if(state == 11) {
		MAT_RESHAPE(b2, 1, 1, 3500);
#ifdef PRINT_DEBUG
			MAT_DUMP(b2, 0);
#endif
		PRINTF("\r\n Layer 11 : Horizontal FC1");

		MAT_RESHAPE(b1, 100, 1, 1);
		zero(b1);
		mat_t *w_ptr = &mat_fc1_wh;
		mat_t *b_ptr = NULL;
		scratch_bak[0] = 12;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, true );
		transition_to(CUR_TASK);
	} else if(state == 12) {
		PRINTF("\r\n Layer 12 : Vertical FC1");

		MAT_RESHAPE(b1, 1, 1, 100);
#ifdef PRINT_DEBUG
			MAT_DUMP(b1,0);
#endif
		MAT_RESHAPE(b2, 500, 1, 1);
		zero(b2);
		mat_t *w_ptr = &mat_fc1_wv;
		mat_t *b_ptr = &mat_fc1_b;
		scratch_bak[0] = 13;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		conv_sparse( w_ptr, b_ptr, b1, b2, 0, false, 0, true);
		transition_to(CUR_TASK);
	} else if(state == 13) {
		MAT_RESHAPE(b2, 1, 1, 500);
#ifdef PRINT_DEBUG
			MAT_DUMP(b2, 0);
#endif

		PRINTF("\r\n Layer 13 : RELU after FC1");

		MAT_RESHAPE(b1, 500, 1);
		scratch_bak[0] = 14;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		relu( b2, 0 );
		transition_to(CUR_TASK);
	} else if(state == 14) {
#ifdef PRINT_DEBUG
		MAT_DUMP(b2, 0);
#endif
		PRINTF("\r\n Layer 14 : FC2");

		MAT_RESHAPE(b1, 2, 1, 1);
		zero(b1);
		mat_t *w_ptr = &mat_fc2_w;
		mat_t *b_ptr = &mat_fc2_b;
		scratch_bak[0] = 0;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		conv_dense( w_ptr, b_ptr, b2, b1, 0 );
		TRANSITION_TO(task_finish);
	}
}

__fram fixed max = 0;
extern uint8_t predict;
void task_finish() {
	fixed max = 0;
	PRINTF("\r\n=====================");
	for(uint16_t i = CUR_SCRATCH[0]; i < 2; i = ++CUR_SCRATCH[0]) {
		fixed v = MAT_GET(b1, i, 0, 0);
		if(v > max) {
			predict = i;
			max = v;
		}
		PRINTF("\r\n%u => %i", i, v);
	}
	PRINTF("\r\n");
	if(predict == 0)
		PRINTF("PREDICTION => %u [No Person in Image]\r\n", predict);
	else if(predict == 1)
		PRINTF("PREDICTION => %u [Person in Image]\r\n", predict);
	PRINTF("\r\n=====================");
	PRINTF("\r\n=====================\r\n");
	TRANSITION_TO(task_exit);
}

void task_exit() {
	/*
	if ( predict == 0 ){
#ifdef enable_debug        	
		PRINTF("STATE 4: No Person in Image. Skipping the rest.\r\n");
#endif
		camaroptera_state = 0;
	}
	else
		camaroptera_state = camaroptera_next_task(2);
	*/
	
	PRINTF("IMG_SEQ INDEX: %u\r\n", index_img_seq);
	PRINTF("FP INDEX: %u\r\n", index_fp);
	PRINTF("FN INDEX: %u\r\n", index_fn);

	//if ( image_sequence[index_img_seq] == 0 ){ 			// EXP1
	if ( read_status == 0 ){ 													// EXP2
		if( false_positives[index_fp] == 0 ){
			camaroptera_state = 0;
			PRINTF("INPUT: NEG | DETECTED: NEG\r\n");
		}
		else{
			camaroptera_state = camaroptera_next_task(2);
			PRINTF("INPUT: NEG | DETECTED: POS\r\n");
			P1OUT |= BIT7;
		}
		index_fp++;
	}
	else{
		P1OUT |= BIT6;
		if ( false_negatives[index_fn] == 0 ){
			camaroptera_state = camaroptera_next_task(2);
			PRINTF("INPUT: POS | DETECTED: POS\r\n");
		}
		else{
			camaroptera_state = 0;
			PRINTF("INPUT: POS | DETECTED: NEG\r\n");
			P1OUT |= BIT7;
			not_sent_count++;
		}
		index_fn ++;
	}
	index_img_seq++;

	P8OUT ^= BIT1; 
	TRANSITION_TO(camaroptera_main);
}


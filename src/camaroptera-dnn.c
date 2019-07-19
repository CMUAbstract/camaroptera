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

#include <libdnn/misc.h>
#include <libdnn/mem.h>
#include <libdnn/state.h>
#include <libdnn/buffer.h>
#include <libdnn/nn.h>
#include <libdnn/nonlinear.h>
#include <libdnn/linalg.h>

#include "headers/conv1.h"
#include "headers/conv2.h"
#include "headers/fc1.h"
#include "headers/fc2.h"
#include "headers/input.h"
#include "camaroptera-dnn.h"

extern uint8_t frame[];
extern uint8_t camaroptera_state;
//#define OLD_PINS

void init();

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////Alapaca Shim///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define MEM_SIZE 0x400
__ro_hifram uint8_t *data_src[MEM_SIZE];
__ro_hifram uint8_t *data_dest[MEM_SIZE];
__ro_hifram unsigned int data_size[MEM_SIZE];
void clear_isDirty() {}

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

	P1DIR = 0x00;
  P2DIR = 0x00;   
  P3DIR = 0x00;   
  P4DIR = 0x00;   
  P5DIR = 0x00;
  P6DIR = 0x00;
  P7DIR = 0x00;   
	
	P8OUT |= BIT1;					// To demarcate start and end of individual runs of the program
	P8OUT &= ~BIT2; 	  		// To demarcate smaller sections of the program
	P8OUT &= ~BIT3;
	P8DIR |= BIT1 + BIT2 + BIT3;

#ifdef OLD_PINS
	P4OUT &= ~BIT7;			// Power to Radio
	P4DIR |= BIT7;
#else
	P4OUT &= ~BIT4;			// Power to Radio
	P4DIR |= BIT4;
#endif


}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////Stacks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
__ro_hifram stack_t st;
__ro_hifram stack_t *mat_stack = &st;

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Weights Matrices/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
__ro_hifram mat_t mat_conv1_wd = {
	.dims = {20, 1, 1, 1},
	.len_dims = 4,
	.strides = {1},
	.data = conv1_wmd,
};

__ro_hifram mat_t mat_conv1_wv = {
	.dims = {1, 20, 5, 1},
	.len_dims = 4,
	.strides = {1},
	.data = conv1_wmv,
};

__ro_hifram mat_t mat_conv1_wh = {
	.dims = {1, 20, 1, 5},
	.len_dims = 4,
	.strides = {1},
	.data = conv1_wmh,
};

__ro_hifram mat_t mat_conv1_b = {
	.dims = {20},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_b,
};

__ro_hifram mat_t  mat_conv2_w = {
	.dims = {CONV2_WM_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_wm,
	.sparse = {
		.dims = {100, 20, 5, 5},
		.len_dims = 4,
		.sizes = conv2_wm_sizes,
		.offsets = conv2_wm_offsets,
	}
};

__ro_hifram mat_t mat_conv2_b = {
	.dims = {100},
	.strides = {1},
	.len_dims = 1,
	.data = conv2_b,
};

__ro_hifram mat_t mat_fc1_w = {
	.dims = {FC1_WM__LEN},
	.len_dims = 1,
	.strides = {1},
	.data = fc1_wm_,
	.sparse = {
		.dims = {500, 5400},
		.len_dims = 2,
		.offsets = fc1_wm__offsets,
		.sizes = fc1_wm__sizes,
	},
};


__ro_hifram mat_t mat_fc1_b = {
	.dims = {500, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc1_b,
};

__ro_hifram mat_t mat_fc2_w = {
	.dims = {2, 500},
	.strides = {500, 1},
	.len_dims = 2,
	.data = fc2_w,
};

__ro_hifram mat_t mat_fc2_b = {
	.dims = {2, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc2_b,
};

__ro_hifram mat_t mat_input = {
	.dims = {1, 80, 60},
	.strides = {4800, 60, 1},
	.len_dims = 3,
	.data = frame,
};

__ro_hifram mat_t buf1 = {.data = LAYER_BUFFER(1)};
__ro_hifram mat_t buf2 = {.data = LAYER_BUFFER(2)};
__ro_hifram mat_t *b1 = &buf1;
__ro_hifram mat_t *b2 = &buf2;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Tasks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void task_init() {
	P8OUT ^= BIT1;
	PRINTF("\r\n========================");
	PRINTF("\r\nInit");

	params.same_padding = false;
	params.size[0] = 1;
	params.size[1] = 2;
	params.size[2] = 2;
	params.stride[0] = 1;
	params.stride[1] = 1;
	params.stride[2] = 1;

	TRANSITION_TO(task_compute);
}

void task_compute() {
	PRINTF("\r\nTask Compute");
	uint16_t state = CUR_SCRATCH[0];
	if(state == 0) {
		MAT_RESHAPE(b2, 1, 80, 60);
		mat_t *mat_input_ptr = &mat_input;
		for(uint16_t i = CUR_SCRATCH[1]; i < 80; i = ++CUR_SCRATCH[1]) {
			for(uint16_t j = CUR_SCRATCH[2]; j < 60; j = ++CUR_SCRATCH[2]) {
				fixed w = MAT_GET(mat_input_ptr, 0, i, j);
				MAT_SET(b2, w, 0, i, j);
			}
			CUR_SCRATCH[2] = 0;
		}

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	} else if(state == 1) {
		MAT_DUMP(b2, 0);
		PRINTF("\r\n Layer 1");

		MAT_RESHAPE(b1, 20, 80, 60);
		mat_t *w_ptr = &mat_conv1_wd;
		mat_t *b_ptr = NULL;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2);

		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_conv)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_s_conv);
	} else if(state == 2) {
		MAT_DUMP(b1, 0);
		PRINTF("\r\n Layer 2");

		MAT_RESHAPE(b2, 20, 80, 56);
		mat_t *w_ptr = &mat_conv1_wh;
		mat_t *b_ptr = NULL;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b2, b1);

		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_depthconv)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_s_depthconv);
	} else if(state == 3) {
		MAT_DUMP(b2, 0);
		PRINTF("\r\n Layer 3");

		MAT_RESHAPE(b1, 20, 76, 56);
		mat_t *w_ptr = &mat_conv1_wv;
		mat_t *b_ptr = &mat_conv1_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2);

		scratch_bak[0] = 4;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_depthconv)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_s_depthconv);
	} else if(state == 4) {
		MAT_DUMP(b1, 0);
		PRINTF("\r\n Layer 4");

		MAT_RESHAPE(b2, 20, 76, 56);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b2, b1);

		scratch_bak[0] = 5;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_relu);
	} else if(state == 5) {
		MAT_DUMP(b2, 0);
		PRINTF("\r\n Layer 5");

		MAT_RESHAPE(b1, 20, 38, 28);
		params.stride[1] = 2;
		params.stride[2] = 2;
		// Assumes src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 6;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_pool)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_pool);
	} else if(state == 6) {
		MAT_DUMP(b1, 0);
		PRINTF("\r\n Layer 6");

		MAT_RESHAPE(b2, 100, 34, 24);
		params.stride[1] = 1;
		params.stride[2] = 1;
		mat_t *w_ptr = &mat_conv2_w;
		mat_t *b_ptr = &mat_conv2_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, NULL, w_ptr, b2, b1);

		scratch_bak[0] = 7;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_conv)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_s_conv);
	} else if(state == 7) {
		MAT_DUMP(b2, 0);
		PRINTF("\r\n Layer 7");

		MAT_RESHAPE(b1, 100, 34, 24);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 8;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_relu);
	} else if(state == 8) {
		MAT_DUMP(b1, 0);
		PRINTF("\r\n Layer 8");

		MAT_RESHAPE(b2, 100, 9, 6);
		params.size[1] = 4;
		params.size[2] = 4;
		params.stride[1] = 4;
		params.stride[2] = 4;
		// Assumes src in that order
		PUSH_STACK(mat_stack, b2, b1);

		scratch_bak[0] = 9;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_pool)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_pool);
	} else if(state == 9) {
		MAT_RESHAPE(b2, 1, 1, 5400);
		MAT_DUMP(b2, 0);
		PRINTF("\r\n Layer 9");

		MAT_RESHAPE(b2, 5400, 1);
		MAT_RESHAPE(b1, 500, 1);
		mat_t *w_ptr = &mat_fc1_w;
		mat_t *b_ptr = &mat_fc1_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2);

		scratch_bak[0] = 10;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_fc)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_s_fc);
	} else if(state == 10) {
		MAT_RESHAPE(b2, 1, 1, 500);
		MAT_DUMP(b2, 0);
		MAT_RESHAPE(b2, 500, 1);
		PRINTF("\r\n Layer 10");

		MAT_RESHAPE(b1, 500, 1);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 11;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_relu);
	} else if(state == 11) {
		MAT_RESHAPE(b1, 1, 1, 500);
		MAT_DUMP(b1, 0);
		MAT_RESHAPE(b1, 500, 1);
		PRINTF("\r\n Layer 11");

		MAT_RESHAPE(b2, 2, 1);
		mat_t *w_ptr = &mat_fc2_w;
		mat_t *b_ptr = &mat_fc2_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b2, b1);

		scratch_bak[0] = 12;
		write_to_gbuf((uint8_t *)(scratch_bak), 
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_d_fc)->info.return_task = TASK_REF(task_compute);
		TRANSITION_TO(task_d_fc);
	}
	scratch_bak[0] = 0;
	scratch_bak[1] = 0;
	scratch_bak[2] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak), 
		(uint8_t *)(CUR_SCRATCH), 3*sizeof(uint16_t));

	TRANSITION_TO(task_finish);
}

__ro_hifram fixed max = 0;
__ro_hifram uint16_t predict = 0;
void task_finish() {
	fixed max = 0;
	PRINTF("\r\nTask Finish");
	PRINTF("\r\n=====================");
	for(uint16_t i = CUR_SCRATCH[0]; i < 2; i = ++CUR_SCRATCH[0]) {
		fixed v = MAT_GET(b2, i, 0);
		if(v > max) {
			predict = i;
			max = v;
		}
		PRINTF("\r\n%u => %i", i, v);
	}
	PRINTF("\r\n=====================");
	PRINTF("\r\n=====================\r\n");
	scratch_bak[0] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak), 
		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));

	TRANSITION_TO(task_exit);
}

void task_exit() {
	camaroptera_state = camaroptera_next_task(2);
	TRANSITION_TO(camaroptera_main);
}


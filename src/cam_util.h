#pragma once

#define __ro_hifram __attribute__((section(".upper.rodata")))
#define __fram __attribute__((section(".persistent")))

#define STATE_CAPTURE 0
#define STATE_DIFF 1
#define STATE_INFER 2
#define STATE_COMPRESS 3
#define STATE_TRANSMIT 4
//#define OLD_PINS

#define enable_debug
//#define cont_power
//#define print_diff
//#define print_image
//#define print_charging
//#define print_packet
#define print_jpeg

//Jpeg quality factor
#define JQ LIBJPEG_QF

// Image differencing parameters
#define P_THR 40
#define P_DIFF_THR 400

#define FRAME_PIXELS 19200 

extern uint8_t frame[];
extern uint8_t frame_jpeg[];
extern uint16_t fp_track;
extern uint16_t fn_track;
extern uint8_t array_for_dummy_dnn[10];

uint8_t camaroptera_next_task(uint8_t current_task);
float camaroptera_wait_for_charge();
void camaroptera_init_lora();
void camaroptera_wait_for_interrupt();
void camaroptera_mode_select(float charge_rate);
uint16_t camaroptera_compression();

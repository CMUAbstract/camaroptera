#include <stdbool.h>
#include <msp430.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <libio/console.h>
#include <libmspbuiltins/builtins.h>

#include <libfixed/fixed.h>
#include <libmat/mat.h>

#include "dnn_layers.h"
#include "cam_framebuffer.h"
#include "camaroptera-dnn.h"
#include "cam_mlkernels.h"
#ifdef EXPERIMENT_MODE
#include "event_headers_for_experiments/experiment_array_1_99_10_10_9.h"
#endif
extern uint8_t camaroptera_state;
extern uint8_t frame_interesting_status;
extern float camaroptera_wait_for_charge(); 
extern void hm01b0_deinit();

__ro_hifram int dnn_layer = 0; /*TODO: Better name -- which part of the DNN evaluating*/

void task_init() {

#ifdef enable_debug          
  PRINTF("\r\n========================");
  PRINTF("\r\nInit");
#endif
  dnn_init(camaroptera_get_framebuffer());
}

#define UPDATE_STATE(s) dnn_layer=s

void task_compute() {

#ifdef enable_debug          
  PRINTF("\r\n=======DNN EVAL=========\r\n");
#endif

#ifdef DUMMY_COMPUTE
  for(int i = 0; i < 1000; i++)
    __delay_cycles(16000);
#else

TOP:

#ifdef enable_debug          
  PRINTF("\r\n=======DNN Layer = %u =========\r\n",dnn_layer);
#endif

  if(dnn_layer == 0) {
    dnn_L0_pool();
    UPDATE_STATE(1);
    goto TOP;
  } else if(dnn_layer == 1) {
    dnn_L1_conv();
    UPDATE_STATE(2);
    goto TOP;
  } else if(dnn_layer == 2) {
    dnn_L2_relu();
    UPDATE_STATE(3);
    goto TOP;
  } else if(dnn_layer == 3) {
    dnn_L3_pool();
    UPDATE_STATE(4);
    goto TOP;
  } else if(dnn_layer == 4) {
    dnn_L4_conv();
    UPDATE_STATE(5);
    goto TOP;
  } else if(dnn_layer == 5) {
    dnn_L5_conv();
    UPDATE_STATE(6);
    goto TOP;
  } else if(dnn_layer == 6) {
    dnn_L6_conv();
    UPDATE_STATE(7);
    goto TOP;
  } else if(dnn_layer == 7) {
    dnn_L7_relu();
    UPDATE_STATE(8);
    goto TOP;
  } else if(dnn_layer == 8) {
    dnn_L8_pool();
    UPDATE_STATE(9);
    goto TOP;
  } else if(dnn_layer == 9) {
    dnn_L9_conv();
    UPDATE_STATE(10);
    goto TOP;
  } else if(dnn_layer == 10) {
    dnn_L10_conv();
    UPDATE_STATE(11);
    goto TOP;
  } else if(dnn_layer == 11) {
    dnn_L11_conv();
    UPDATE_STATE(12);
    goto TOP;
  } else if(dnn_layer == 12) {
    dnn_L12_relu();
    UPDATE_STATE(14);
    goto TOP;
  } else if(dnn_layer == 14) {
    dnn_L14_fc();
    UPDATE_STATE(15);
    goto TOP;
  } else if(dnn_layer == 15) {
    dnn_L15_relu();
    UPDATE_STATE(16);
    goto TOP;
  } else if(dnn_layer == 16) {
    dnn_L16_fc();
    UPDATE_STATE(0);
  } 

#endif
}

__fram uint8_t prediction = 0;
void task_finish() {

  PRINTF("\r\n=====================");
  prediction = dnn_get_class_result();  

#ifdef enable_debug          
  PRINTF("\r\n");
  PRINTF("PREDICTION => %u \r\n", prediction);
  PRINTF("\r\n=====================");
  PRINTF("\r\n=====================\r\n");
#endif
}

void task_exit() {

//   if ( prediction == 0 ){
// #ifdef enable_debug          
//     PRINTF("STATE 4: Nothing detected \r\n");
// #endif
//     camaroptera_state = 0;
//   } else{
    camaroptera_state = camaroptera_next_task(2);
// }

#ifndef cont_power
    camaroptera_wait_for_charge();       //Wait to charge up 
#endif

#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P6OUT &= ~BIT5;     // Running: Infer
#endif
  //TRANSITION_TO(camaroptera_main); return
}


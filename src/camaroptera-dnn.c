#include <stdbool.h>
#include <msp430.h>
#include <stdlib.h>
#include <string.h>

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

__ro_hifram uint16_t fp_track = 0;
__ro_hifram uint16_t fn_track = 0;
__fram uint8_t predict;
__ro_hifram int dnn_layer = 0; /*TODO: Better name -- which part of the DNN evaluating*/
__ro_hifram uint8_t index_for_dummy_dnn = 0;

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

/*
TASK(1, camaroptera_main);
TASK(2, task_init);
TASK(3, task_compute);
TASK(4, task_finish);
TASK(5, task_exit);
*/
//ENTRY_TASK(camaroptera_main)

//void _entry_task(); 
//TASK(0, _entry_task); 
//void _entry_task() { 

  //TRANSITION_TO(camaroptera_main); 

//}

//INIT_FUNC(init)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Setup///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



void task_init() {

#ifdef enable_debug          
  PRINTF("\r\n========================");
  PRINTF("\r\nInit");
#endif
  dnn_init((void *)camaroptera_get_framebuffer());
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
    dnn_L1_pooling();
    UPDATE_STATE(1);
    goto TOP;
  } else if(dnn_layer == 1) {
    dnn_L2_conv();
    UPDATE_STATE(2);
    goto TOP;
  }else if(dnn_layer == 2) {
    dnn_L3_conv();
    UPDATE_STATE(3);
    goto TOP;
  } else if(dnn_layer == 3) {
    dnn_L4_conv();
    UPDATE_STATE(4);
    goto TOP;
  } else if(dnn_layer == 4) {
    dnn_L5_relu();
    UPDATE_STATE(5);
    goto TOP;
  } else if(dnn_layer == 5) {
    dnn_L6_pooling();
    UPDATE_STATE(6);
    goto TOP;
  } else if(dnn_layer == 6) {
    dnn_L7_conv();
    UPDATE_STATE(7);
    goto TOP;
  }else if(dnn_layer == 7) {
    dnn_L8_conv();
    UPDATE_STATE(8);
    goto TOP;
  } else if(dnn_layer == 8) {
    dnn_L9_conv();
    UPDATE_STATE(9);
    goto TOP;
  } else if(dnn_layer == 9) {
    dnn_L10_relu();
    UPDATE_STATE(10);
    goto TOP;
  } else if(dnn_layer == 10) {
    dnn_L11_pooling();
    UPDATE_STATE(11);
    goto TOP;
  } else if(dnn_layer == 11) {
    dnn_L12_fc1();
    UPDATE_STATE(12);
    goto TOP;
  } else if(dnn_layer == 12) {
    dnn_L13_fc2();
    UPDATE_STATE(13);
    goto TOP;
  } else if(dnn_layer == 13) {
    dnn_L14_relu();
    UPDATE_STATE(14);
    goto TOP;
  } else if(dnn_layer == 14) {
    dnn_L15_fc3();
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
  if(prediction == 0)
    PRINTF("PREDICTION => %u [No Person in Image]\r\n", prediction);
  else if(prediction == 1)
    PRINTF("PREDICTION => %u [Person in Image]\r\n", prediction);
  PRINTF("\r\n=====================");
  PRINTF("\r\n=====================\r\n");
#endif
}

void task_exit() {

#ifdef EXPERIMENT_MODE
  
  PRINTF("fp_track:%u  | fn_track:%u\r\n", fp_track, fn_track);
  if (frame_interesting_status){
    if(false_negatives[fn_track]){
      prediction = 0;
      PRINTF("--->>False Negative.\r\n");
    }
    else{
      prediction = 1;
      PRINTF("--->>True Positive.\r\n");
    }
    fn_track ++;
  }
  else{
    if(false_positives[fp_track]){
      prediction = 1;
      PRINTF("--->>False Positive.\r\n");
    }
    else{
      prediction = 0;
      PRINTF("--->>True Negative.\r\n");
    }
    fp_track ++;
  }
    
#endif 


  if ( prediction == 0 ){
#ifdef enable_debug          
    PRINTF("STATE 4: No Person in Image. Skipping the rest.\r\n");
#endif
    camaroptera_state = 0;
  } else{
    camaroptera_state = camaroptera_next_task(2);
  }

#ifndef cont_power
    camaroptera_wait_for_charge();       //Wait to charge up 
#endif

#ifdef EXPERIMENT_MODE
  P5OUT &= ~BIT5;     // Signal end 
  P6OUT &= ~BIT5;     // Running: Infer
#endif
  //TRANSITION_TO(camaroptera_main); return
}


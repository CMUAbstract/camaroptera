#include "headers/conv_1_weight.h"
#include "headers/conv_4_weight.h"
#include "headers/conv_7_weight.h"
#include "headers/fc_0_weight.h"
#include "headers/fc_0_bias.h"
#include "headers/fc_2_weight.h"
#include "headers/fc_2_bias.h"

#include "camaroptera-dnn.h"
#include "dnn_layers.h"
#include "cam_mlkernels.h"

#include <libfixed/fixed.h>
#include <libmat/mat.h>
#include <stddef.h>
#include <stdbool.h>

#define CLASSES 2

__ro_fram mat_t mat_conv1_weight = {
  .dims = {8, 1, 5, 5},
  .strides = {25, 25, 5, 1},
  .len_dims = 4,
  .data = conv_1_weight
};

__ro_fram mat_t mat_conv2_weight = {
  .dims = {16, 8, 5, 5},
  .strides = {200, 25, 5, 1},
  .len_dims = 4,
  .data = conv_4_weight
};

__ro_fram mat_t mat_conv3_weight = {
  .dims = {32, 16, 5, 5},
  .strides = {400, 25, 5, 1},
  .len_dims = 4,
  .data = conv_7_weight
};

__ro_fram mat_t mat_input = {
  .dims = {1, 120, 160},
  .strides = {19200, 160, 1},
  .len_dims = 3,
  .data = NULL,
};

__ro_fram mat_t mat_fc0_weight = {
  .dims = {FC_0_WEIGHT_SIZE},
  .strides = {1},
  .len_dims = 4,
  .data = fc_2_weight,
  .sparse = {
    .dims = {64, 1, 1, 3072},
    .len_dims = 4,
    .offsets = fc_0_weight_index,
    .sizes = fc_0_weight_indptr,
  }
};

__ro_fram mat_t mat_fc0_bias = {
  .dims = {64, 1},
  .strides = {1, 1},
  .len_dims = 2,
  .data = fc_0_bias,
};

__ro_fram mat_t mat_fc2_weight = {
  .dims = {CLASSES, 1, 1, 64},
  .strides = {64, 64, 64, 1},
  .len_dims = 4,
  .data = fc_2_weight,
};

__ro_fram mat_t mat_fc2_bias = {
  .dims = {CLASSES, 1},
  .strides = {1, 1},
  .len_dims = 2,
  .data = fc_2_bias,
};

__fram mat_t buf1 = {.data = inference_buffer[0]};
__fram mat_t buf2 = {.data = inference_buffer[1]};
__fram mat_t *b1 = &buf1;
__fram mat_t *b2 = &buf2;

void dnn_init(void *data){
  mat_input.data = (fixed *)data; 
}

void dnn_L1_pooling(){
  MAT_RESHAPE(b1, 1, 120, 160);
  mat_t *mat_input_ptr = &mat_input;
  normalize( mat_input_ptr, b1 );
  MAT_RESHAPE(b2, 1, 60, 80);
  pooling( b1, b2, 9, 2, 2 ); 
}

void dnn_L2_conv(){

  MAT_RESHAPE(b1, 8, 30, 40);
  zero(b1);
  mat_t *w_ptr = &mat_conv1_weight;
  mat_t *b_ptr = NULL;
  conv_dense( w_ptr, b_ptr, b2, b1, 0);

}

void dnn_L3_relu(){

    MAT_RESHAPE(b1, 8, 56, 76);
    relu( b1, 0 );

}

void dnn_L4_pooling(){

  MAT_RESHAPE(b2, 8, 28, 38);
  zero(b2);
  pooling( b1, b2, 0, 2, 2 );

}

void dnn_L5_conv(){

  MAT_RESHAPE(b1, 16, 28, 38);
  zero(b1);
  mat_t *w_ptr = &mat_conv2_weight;
  mat_t *b_ptr = NULL;
  conv_dense( w_ptr, b_ptr, b2, b1, 0);

}

void dnn_L6_relu(){

    MAT_RESHAPE(b1, 8, 24, 34);
    relu( b1, 0 );

}

void dnn_L7_pooling(){

  MAT_RESHAPE(b2, 8, 12, 17);
  zero(b2);
  pooling( b1, b2, 0, 2, 2 );

}

void dnn_L8_conv(){

  MAT_RESHAPE(b1, 32, 12, 17);
  zero(b1);
  mat_t *w_ptr = &mat_conv3_weight;
  mat_t *b_ptr = NULL;
  conv_dense( w_ptr, b_ptr, b2, b1, 0);

}

void dnn_L9_relu(){

    MAT_RESHAPE(b1, 32, 8, 12);
    relu( b1, 0 );

}

void dnn_L10_fc1(){

    MAT_RESHAPE(b1, 1, 1, 3072);
    MAT_RESHAPE(b2, 64, 1, 1);
    zero(b1);
    mat_t *w_ptr = &mat_fc0_weight;
    mat_t *b_ptr = &mat_fc0_bias;
    conv_sparse( w_ptr, b_ptr, b1, b2, 0, false, 0, true );

}

void dnn_L11_relu(){

    MAT_RESHAPE(b2, 1, 1, 64);
    relu( b2, 0 );

}

void dnn_L12_fc2(){
    
    MAT_RESHAPE(b1, CLASSES, 1, 1);
    zero(b1);
    mat_t *w_ptr = &mat_fc2_weight;
    mat_t *b_ptr = &mat_fc2_bias;
    conv_dense( w_ptr, b_ptr, b2, b1, 0 );

}

uint8_t dnn_get_class_result(){
  
  fixed max = 0;
  uint8_t predict = 0;
  for( uint16_t i = 0; i < CLASSES; i++ ) {

    fixed v = MAT_GET(b1, i, 0, 0);
    if(v > max) {

      predict = i;
      max = v;

    }

  }
  return predict;

}

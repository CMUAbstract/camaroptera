#include "headers_30x40/conv1.h"
#include "headers_30x40/conv2.h"
#include "headers_30x40/fc1.h"
#include "headers_30x40/fc2.h"

#include "camaroptera-dnn.h"
#include "dnn_layers.h"
#include "cam_mlkernels.h"

#include <libfixed/fixed.h>
#include <libmat/mat.h>
#include <stddef.h>
#include <stdbool.h>

__ro_fram mat_t mat_conv1_wd = {
  .dims = {CONV1_WMD_LEN},
  .len_dims = 1,
  .strides = {1},
  .data = conv1_wmd,
  .sparse = {
    .dims = {20, 1, 1, 1},
    .len_dims = 4,
    .sizes = (uint16_t *) conv1_wmd_sizes,
    .offsets = (uint16_t *) conv1_wmd_offsets
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
    .sizes = (uint16_t *) conv1_wmv_sizes,
    .offsets = (uint16_t *) conv1_wmv_offsets
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
    .sizes = (uint16_t *) conv1_wmh_sizes,
    .offsets = (uint16_t *) conv1_wmh_offsets
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
    .sizes = (uint16_t *) conv2_wmd_sizes,
    .offsets = (uint16_t *) conv2_wmd_offsets
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
    .sizes = (uint16_t *) conv2_wmv_sizes,
    .offsets = (uint16_t *) conv2_wmv_offsets
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
    .sizes = (uint16_t *) conv2_wmh_sizes,
    .offsets = (uint16_t *) conv2_wmh_offsets
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
    .offsets = (uint16_t *) fc1_wmh_offsets,
    .sizes = (uint16_t *) fc1_wmh_sizes,
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
    .offsets = (uint16_t *) fc1_wmv_offsets,
    .sizes = (uint16_t *) fc1_wmv_sizes,
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
  .data = (fixed *) fc2_w,
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
  .data = NULL,
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
  MAT_RESHAPE(b2, 1, 30, 40);
  pooling( b1, b2, 1, 4, 4 ); 
}

void dnn_L2_conv(){

  MAT_RESHAPE(b1, 20, 30, 40);
  zero(b1);
  mat_t *w_ptr = &mat_conv1_wd;
  mat_t *b_ptr = NULL;
  conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, false);

}

void dnn_L3_conv(){

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

}

void dnn_L4_conv(){

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

}

void dnn_L5_relu(){

    MAT_RESHAPE(b2, 20, 26, 36);
    relu( b1, 0 );

}

void dnn_L6_pooling(){

    MAT_RESHAPE(b2, 20, 13, 18);
    zero(b2);
    pooling( b1, b2, 0, 2, 2 );

}

void dnn_L7_conv(){

    MAT_RESHAPE(b1, 100, 13, 18);
    zero(b1);
    mat_t *w_ptr = &mat_conv2_wd;
    mat_t *b_ptr = NULL;
    conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, false);

}

void dnn_L8_conv(){

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
}

void dnn_L9_conv(){
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

}

void dnn_L10_relu(){

    relu( b1, 0 );

}

void dnn_L11_pooling(){

    MAT_RESHAPE(b2, 100, 5, 7);
    pooling( b1, b2, 0, 2, 2 );

}

void dnn_L12_fc1(){

    MAT_RESHAPE(b2, 1, 1, 3500);
    MAT_RESHAPE(b1, 100, 1, 1);
    zero(b1);
    mat_t *w_ptr = &mat_fc1_wh;
    mat_t *b_ptr = NULL;
    conv_sparse( w_ptr, b_ptr, b2, b1, 0, false, 0, true );

}

void dnn_L13_fc2(){

    MAT_RESHAPE(b1, 1, 1, 100);
    MAT_RESHAPE(b2, 500, 1, 1);
    zero(b2);
    mat_t *w_ptr = &mat_fc1_wv;
    mat_t *b_ptr = &mat_fc1_b;
    conv_sparse( w_ptr, b_ptr, b1, b2, 0, false, 0, true);

}

void dnn_L14_relu(){

    MAT_RESHAPE(b2, 1, 1, 500);
    MAT_RESHAPE(b1, 500, 1);
    relu( b2, 0 );

}

void dnn_L15_fc3(){
    
    MAT_RESHAPE(b1, 2, 1, 1);
    zero(b1);
    mat_t *w_ptr = &mat_fc2_w;
    mat_t *b_ptr = &mat_fc2_b;
    conv_dense( w_ptr, b_ptr, b2, b1, 0 );

}

uint8_t dnn_get_class_result(){
  
  fixed max = 0;
  uint8_t predict = 0;
  for( uint16_t i = 0; i < 2; i++ ) { /*BML: Why 2?*/

    fixed v = MAT_GET(b1, i, 0, 0);
    if(v > max) {

      predict = i;
      max = v;

    }

  }
  return predict;

}

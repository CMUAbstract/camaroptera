#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <libio/console.h>

#include <libfixed/fixed.h>
#include <libmat/mat.h>

#include "lenet.h"

extern uint8_t input[];

//__ro_hifram uint8_t frame[19200];

__ro_hifram fixed inference_buffer[BUFFER_NUM][BUFFER_SIZE];

void zero( mat_t *buffer ){
  // -----   INPUTS:
  //          buffer <= buffer to fill with zeros
  // NOTE: All inputs need to contain the appropriate dims, this function WILL NOT compute the output dims or check for dims matching, yet.
  // -----   OUTPUT:
  //         None
  uint16_t depth, height, width;
  uint16_t i,j,k;

  depth = buffer->dims[0];
  height = buffer->dims[1];
  width = buffer->dims[2];
  
  for( i = 0; i < depth; i++ )
    for( j = 0; j < height; j++ )
      for( k = 0; k < width; k++ )
        MAT_SET( buffer, 0, i, j, k );
  
  }



void normalize( mat_t *input_buffer, mat_t *dest_buffer ){
  // -----   INPUTS:
  //          input_buffer <= buffer containing input as a mat_t pointer (depth, height, width)
  //          dest_buffer <= buffer to store output as a mat_t pointer (depth, height, width)
  // NOTE: All inputs need to contain the appropriate dims, this function WILL NOT compute the output dims or check for dims matching, yet.
  // -----   OUTPUT:
  //         None
  
  uint16_t o_depth, o_height, o_width;
  
  o_depth = dest_buffer->dims[0];
  o_height = dest_buffer->dims[1];
  o_width = dest_buffer->dims[2];
  
  uint16_t depth, height, width, d_idx, h_idx;

  fixed temp;
  fixed threshold = F_LIT(256);

  // PRINTF("NORMALIZING IMAGE\r\n");
  // PRINTF("IMAGE DIMS: (%i, %i, %i)", o_depth, o_height, o_width);
  
  for( depth = 0; depth < o_depth; depth++ ){
    d_idx = depth * o_height;
    for( height = 0; height < o_height; height++ ){
      h_idx = height * o_width;
      for( width = 0; width < o_width; width++ ){
        temp = MAT_GET( input_buffer, depth, height, width );
        temp = F_DIV(temp, threshold);
        MAT_SET( dest_buffer, temp, depth, height, width);
      }
    }
  }
  
}

void relu( mat_t *src_buffer, uint16_t threshold ){
  // -----   INPUTS:
  //          src_buffer <= buffer that contains input as a mat_t pointer
  //          threshold <= all src_buffer elements below "threshold" will be set to zero
  // NOTE: All inputs need to contain the appropriate dims, this function WILL NOT compute the output dims or check for dims matching, yet.
  // -----   OUTPUT:
  //         None

  uint16_t i_depth, i_height, i_width;

  i_depth = src_buffer->dims[0];
  i_height = src_buffer->dims[1];
  i_width = src_buffer->dims[2];
  
  uint16_t depth, height, width;

  fixed f_threshold = F_LIT(threshold);
  
  // PRINTF("RELU LAYER\r\n");
  // PRINTF("INPUT LAYER DIMS: (%i, %i, %i)", i_depth, i_height, i_width);
  for( depth = 0; depth < i_depth; depth++ )
    for( height = 0; height < i_height; height++ )
      for( width = 0; width < i_width; width++ )
        if( MAT_GET(src_buffer, depth, height, width) < f_threshold )
          MAT_SET(src_buffer, 0, depth, height, width);  
}

void pooling( mat_t *src_buffer, mat_t *dest_buffer, uint8_t type, uint8_t kernel_size, uint8_t stride  ){
  // -----   INPUTS:
  //          src_buffer <= buffer that contains input as a mat_t pointer
  //          dest_buffer <= buffer to store output as a mat_t pointer
  //          type <= [0] max-pooling [1] average-pooling
  //         kernel_size, stride <= shape and stride of pooling mask
  // NOTE: All inputs need to contain the appropriate dims, this function WILL NOT compute the output dims or check for dims matching, yet.
  // -----   OUTPUT:
  //         None
  
  // PRINTF("POOLING LAYER\r\n");
  uint16_t filter_id, depth, height, width, filter_ht, filter_wd;

  uint16_t i_depth, i_height, i_width, h_idx, w_idx;

  i_depth = src_buffer->dims[0];
  i_height = src_buffer->dims[1];
  i_width = src_buffer->dims[2];

  // PRINTF("\r\nINPUT DIMS:(%i, %i, %i)\r\n", i_depth, i_height, i_width);
  
  uint16_t o_depth = dest_buffer->dims[0];
  uint16_t o_height = dest_buffer->dims[1];
  uint16_t o_width = dest_buffer->dims[2];
  
  // PRINTF("OUTPUT DIMS:(%i, %i, %i)\r\n", o_depth, o_height, o_width);

  fixed temp, pool_result, avg_denom;

  avg_denom = kernel_size*kernel_size;

  for( depth = 0; depth < o_depth; depth++ ){
    for( height = 0; height < o_height; height++ ){
      for( width = 0; width < o_width; width++ ){
        
        if (type == 0)
          pool_result = F_MIN;
        else if (type == 1)
          pool_result = 0;
        
        h_idx = (height) * stride;
        w_idx = (width) * stride;
        for( filter_ht = 0; filter_ht < kernel_size; filter_ht++ ){
          for( filter_wd = 0; filter_wd < kernel_size; filter_wd++ ){
            if( (h_idx+filter_ht) < i_height && (w_idx+filter_wd) < i_width )
              temp = MAT_GET(src_buffer, depth, h_idx+filter_ht, w_idx+filter_wd);
            else
              temp = F_MIN;
            //PRINTF("(%i,%i) %i | ", h_idx+filter_ht, w_idx+filter_wd, temp);
            if( type == 0 && temp > pool_result) // MAXPOOLING
              pool_result = temp;
            else if( type == 1 )                 // AVGPOOLING
              pool_result = F_ADD( pool_result, temp ); 
          }
        }
        //PRINTF("%i | ", pool_result);
        if( type == 1 )                          
          pool_result = pool_result/avg_denom;
        MAT_SET(dest_buffer, pool_result, depth, height, width);
        //PRINTF("\r\n");
      }
      //PRINTF("\r\n");
    }
  }
}

void conv_dense( mat_t *weight, mat_t *bias, mat_t *src_buffer, mat_t *dest_buffer, uint16_t stride ){
  // -----   INPUTS:
  //          weight <= dense weight matrix as a mat_t pointer
  //          src_buffer <= buffer that contains input as a mat_t pointer
  //          dest_buffer <= buffer to store output as a mat_t pointer
  //          stride <= stride for the filters
  // NOTE: All inputs need to contain the appropriate dims, this function WILL NOT compute the output dims or check for dims matching, yet.
  // -----   OUTPUT:
  //         None

  fixed sum_temp = 0;

  uint16_t filter_id, depth, height, width, filter_ht, filter_wd;

  uint16_t i_depth, i_height, i_width, filters, f_depth, f_height, f_width;

  i_depth = src_buffer->dims[0];
  i_height = src_buffer->dims[1];
  i_width = src_buffer->dims[2];

  filters = weight->dims[0];
  f_depth = weight->dims[1];
  f_height = weight->dims[2];
  f_width = weight->dims[3];

  // PRINTF("DENSE CONV\r\n");
  // PRINTF("\r\nINPUT DIMS:(%i, %i, %i)\r\n", i_depth, i_height, i_width);
  
  // PRINTF("FILTER DIMS:(%i, %i, %i, %i)\r\n", filters, f_depth, f_height, f_width);

  uint16_t o_depth = dest_buffer->dims[0];
  uint16_t o_height = dest_buffer->dims[1];
  uint16_t o_width = dest_buffer->dims[2];
  
  // PRINTF("OUTPUT DIMS:(%i, %i, %i)\r\n", o_depth, o_height, o_width);
  
  for( filter_id = 0; filter_id < filters; filter_id++ ){
    
    //PRINTF("Convolving %i\r\n", filter_id);
    for( depth = 0; depth < f_depth; depth++ ){
      for( height = 0; height < o_height; height++ ){
        for( width = 0; width < o_width; width++ ){
          for( filter_ht = 0; filter_ht < f_height; filter_ht++ ){
            for( filter_wd = 0; filter_wd < f_width; filter_wd++ ){
                sum_temp += F_MUL( MAT_GET(src_buffer, depth, height+filter_ht, width+filter_wd), MAT_GET(weight, filter_id, depth, filter_ht, filter_wd) );
            }
          }
          if (bias != NULL)
            sum_temp += MAT_GET(bias, filter_id);
          MAT_SET(dest_buffer, sum_temp, filter_id, height, width);
          sum_temp = 0;
        }
      }
    }
  }
}

void conv_sparse( mat_t *weight, mat_t *bias, mat_t *src_buffer, mat_t *dest_buffer, uint16_t stride, bool depthwise, uint16_t depth_id, bool fc ){
  // -----   INPUTS:
  //          weight <= sparse weight matrix as a mat_t pointer
  //          src_buffer <= buffer that contains input as a mat_t pointer
  //          dest_buffer <= buffer to store output as a mat_t pointer
  //          stride <= stride for the filters
  // NOTE: All inputs need to contain the appropriate dims, this function WILL NOT compute the output dims or check for dims matching, yet.
  // -----   OUTPUT:
  //         None

  uint16_t filter_id, height, width, filter_ht, filter_wd, weight_idx;

  uint16_t i_depth, i_height, i_width, o_depth, o_height, o_width, filters, f_depth, f_height, f_width, total_elements, element_id, f_offset;

  fixed weight_temp, sum_temp;

  if( src_buffer->len_dims == 3){
    i_depth = src_buffer->dims[0];
    i_height = src_buffer->dims[1];
    i_width = src_buffer->dims[2];
  }
  else{
    i_depth = 1;
    i_height = src_buffer->dims[0];
    i_width = src_buffer->dims[1];
  }

  if( weight->sparse.len_dims == 4){
    filters = weight->sparse.dims[0];
    f_depth = weight->sparse.dims[1];
    f_height = weight->sparse.dims[2];
    f_width = weight->sparse.dims[3];
  }
  else{
    filters = 1;
    f_depth = weight->sparse.dims[0];
    f_height = weight->sparse.dims[1];
    f_width = weight->sparse.dims[2];
  }

  if( dest_buffer->len_dims == 3){
    o_depth = dest_buffer->dims[0];
    o_height = dest_buffer->dims[1];
    o_width = dest_buffer->dims[2];
  }
  else{
    o_depth = 1;
    o_height = dest_buffer->dims[0];
    o_width = dest_buffer->dims[1];
  }

  // PRINTF("SPARSE CONV\r\n");
  // PRINTF("\r\nINPUT DIMS:(%i, %i, %i)\r\n", i_depth, i_height, i_width);
  
  // PRINTF("FILTER DIMS:(%i, %i, %i, %i)\r\n", filters, f_depth, f_height, f_width);

  // PRINTF("OUTPUT DIMS:(%i, %i, %i)\r\n", o_depth, o_height, o_width);
  
  bool first = true;
  f_offset = 0;

  for( filter_id = 0; filter_id < filters; filter_id++ ){
    
    if(fc)
      total_elements = weight->sparse.sizes[filter_id+1] - f_offset;
    else
      total_elements = weight->sparse.sizes[filter_id];

    weight_idx = 0;

    first = true;
  
    // PRINTF("FILTER %i: %i elements", filter_id, total_elements);

    for( element_id = 0; element_id < total_elements; element_id++ ){

      weight_temp = MAT_GET( weight, f_offset+element_id);
      if(fc)
        weight_idx = weight->sparse.offsets[f_offset+element_id];  
      else  
        weight_idx += weight->sparse.offsets[f_offset+element_id];
      f_depth = weight_idx / (f_height*f_width);
      filter_ht = (weight_idx % (f_height*f_width)) / f_width;
      filter_wd = (weight_idx % (f_height*f_width)) % f_width;
      //PRINTF("%i(%i,%i,%i) | ", weight_temp, f_depth, filter_ht, filter_wd);
    
      for( height = 0; height < o_height; height++ ){
        for( width = 0; width < o_width; width++ ){
            if(depthwise){
              sum_temp = F_MUL( weight_temp, MAT_GET( src_buffer, depth_id, height+filter_ht, width+filter_wd ) );
              if(!first)
                sum_temp += MAT_GET( dest_buffer, depth_id, height, width );
              MAT_SET( dest_buffer, sum_temp, depth_id, height, width );
            }
            else{
              sum_temp = F_MUL( weight_temp, MAT_GET( src_buffer, f_depth, height+filter_ht, width+filter_wd ) );
            //  PRINTF("(%i,%i)[%i,%i] |", w, sum_temp, height, width);
              if(!first){
                sum_temp += MAT_GET( dest_buffer, filter_id, height, width );
              //  PRINTF("%i |", MAT_GET( dest_buffer, filter_id, height, width) );
              }
            //  PRINTF("%i |", sum_temp);
              MAT_SET( dest_buffer, sum_temp, filter_id, height, width );
            }
        }
      }
      //PRINTF("\r\n\n");
      first = false;
    }

    
    if(bias != NULL){
      //PRINTF("BIASES:\r\n");
      for( height = 0; height < o_height; height++ ){
        for( width = 0; width < o_width; width++ ){
          if(depthwise){
            sum_temp = MAT_GET( dest_buffer, depth_id, height, width );
            sum_temp += MAT_GET( bias, depth_id );
            MAT_SET( dest_buffer, sum_temp, depth_id, height, width );
          }
          else{
            sum_temp = MAT_GET( dest_buffer, filter_id, height, width );
              //PRINTF("(%i,%i)[%i,%i] |", sum_temp, MAT_GET( bias, filter_id), height, width);
            sum_temp += MAT_GET( bias, filter_id );
            MAT_SET( dest_buffer, sum_temp, filter_id, height, width );  
            }
          }
      }
    }

    f_offset += total_elements;
    // PRINTF("\r\n");
  }
}


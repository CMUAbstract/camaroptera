#include "main.h"
#include <libfixed/fixed.h>
#include <libmat/mat.h>

#define XSTR(x) STR(x)
#define STR(x) #x

#ifndef WORKLOAD
#error "Define Workload"
#endif

#define DCONV 1
#define DMM 2
#define DMV 3
#define SCONV 4
#define SMM 5
#define SMV 6

#pragma message "Workload is: " XSTR(WORKLOAD)
#include "common/mat.h"

#if WORKLOAD == DCONV

void task_init() {
	uint16_t rows = MAT_GET_DIM((&mat_a_dense), 0);
	uint16_t cols = MAT_GET_DIM((&mat_a_dense), 1);
	uint16_t frows = MAT_GET_DIM((&mat_c_dense), 2);
	uint16_t fcols = MAT_GET_DIM((&mat_c_dense), 3);
	
	MAT_RESHAPE(b1, 1, rows, cols); 		// REDUNDANT : Keep consistent w/ MANIC
	MAT_RESHAPE(b1, (rows - frows + 1), (cols - fcols + 1));
	//MAT_RESHAPE(b3, (rows - frows + 1), (cols - fcols + 1));

	// REDUNDANT : Keep consistent w/ MANIC
	mat_t *mat_input_ptr = &mat_a_dense;
	for(uint16_t i = 0; i < rows; i++ ) {
		for(uint16_t j = 0; j < cols; j ++) {
			MAT_SET(b1, MAT_GET(mat_input_ptr, i, j), 0, i, j);
		}
	}
}

void task_compute() {
	mat_t *src = &mat_a_dense;
	mat_t *dest = b1;
	//mat_t *inter = b3;
	mat_t *filter = &mat_c_dense;

	uint16_t rows = MAT_GET_DIM(dest, 0);
	uint16_t cols = MAT_GET_DIM(dest, 1);
	//MAT_RESHAPE(inter, rows, cols);

	uint16_t flayers = MAT_GET_DIM(filter, 1);
	uint16_t frows = MAT_GET_DIM(filter, 2);
	uint16_t fcols = MAT_GET_DIM(filter, 3);

	for(uint16_t i = 0; i < flayers; i++) {
		fixed *dest_ptr = dest->data;
		for(uint16_t j = 0; j < rows; j++) {
			for(uint16_t k = 0; k < cols; k++) {
				fixed w = 0;
				fixed *filter_ptr = MAT_PTR(filter, i, 0, 0);
				fixed *src_ptr = MAT_PTR(src, i, j, k);
				//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	
				for(uint16_t l = 0; l < frows; l++) {
					for(uint16_t m = 0; m < fcols; m++) {
						//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	
						w = F_ADD(w, F_MUL(*src_ptr, *filter_ptr));
						//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	
						//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);	
						filter_ptr++;
						//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);	
						//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);	
						src_ptr++;
						//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);	
					}
				}
				*dest_ptr++ = w; 
				//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	
			}
		}
}

}

fixed workload_run(){ 
	task_init();
	task_compute();
	
	return MAT_GET( b1, 0, 0);  
}

#elif WORKLOAD == DMM

#define min(a, b) (((a) < (b)) ? (a) : (b))

void task_init() {
	// This assumes a square matrix
	uint16_t rows = MAT_GET_DIM((&mat_a_dense), 0);
	uint16_t cols = MAT_GET_DIM((&mat_a_dense), 1);
	MAT_RESHAPE(b1, rows, cols);
	//MAT_RESHAPE(b2, rows, cols);
	mat_t *mat_input_ptr = &mat_a_dense;
	for(uint16_t i = 0; i < rows; i++) {
		for(uint16_t j = 0; j < cols; j++) {
			MAT_SET(b1, MAT_GET(mat_input_ptr, i, j), i, j);
		}
	}
}

void task_compute() {
	// PLACE CODE HERE
	mat_t *src = &mat_a_dense;
	mat_t *dest = b1;
	mat_t *filter = &mat_a_dense;

	uint16_t rows = MAT_GET_DIM(filter, 0);
	uint16_t cols = MAT_GET_DIM(filter, 1);
	uint16_t dcols = MAT_GET_DIM(dest, 1);

	for(uint16_t i = 0; i < rows; i += BLOCK_SIZE) {
		for(uint16_t j = 0; j < dcols; j += BLOCK_SIZE) {
			for(uint16_t k = 0; k < cols; k += BLOCK_SIZE) {
				fixed *filter_ptr = filter->data + i * cols + k;
				for(uint16_t l = i; l < min(i + BLOCK_SIZE, rows); l++) {
					fixed *dest_ptr = dest->data + l * dcols + j;
					for(uint16_t m = j; m < min(j + BLOCK_SIZE, dcols); m++) {
						fixed w = 0;
						fixed *src_ptr = src->data + dcols * k + m;
						fixed *cur_filter_ptr = filter_ptr;
						for(uint16_t n = k; n < min(k + BLOCK_SIZE, cols); n++) {
							w = F_ADD(w, F_MUL(*cur_filter_ptr, *src_ptr));
							src_ptr += dcols;
							cur_filter_ptr++;
						}
						*dest_ptr++ += w;
					}
					filter_ptr += cols;
				}
			}
		}
}

}
//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	


int workload_run(){
	task_init();
	task_compute();
	
	return MAT_GET( b1, 0, 0);  
}

#elif WORKLOAD==DMV

#define min(a, b) (((a) < (b)) ? (a) : (b))

void task_init() {
	MAT_RESHAPE(b1, MAT_GET_DIM((&mat_b_dense), 0), 1);
	//MAT_RESHAPE(b2, MAT_GET_DIM((&mat_a_dense), 0), 1);
	//MAT_RESHAPE(b3, MAT_GET_DIM((&mat_a_dense), 0), 1);
	mat_t *mat_input_ptr = &mat_b_dense;
	for(uint16_t i = 0; i < MAT_GET_DIM((&mat_b_dense), 0); 
		i++) {
		MAT_SET(b1, MAT_GET(mat_input_ptr, i, 0), i, 0);
	}
}

void task_compute() {
	// PLACE CODE HERE
	mat_t *src = &mat_b_dense;
	mat_t *dest = b1;
	mat_t *filter = &mat_a_dense;

	uint16_t rows = MAT_GET_DIM(filter, 0);
	uint16_t cols = MAT_GET_DIM(filter, 1);

	for(uint16_t i = 0; i < rows; i += BLOCK_SIZE) {
		for(uint16_t j = 0; j < cols; j += BLOCK_SIZE) {
			fixed *dest_ptr = dest->data + i;
			for(uint16_t k = i; k < min(i + BLOCK_SIZE, rows); k++) {
				fixed *filter_ptr = filter->data + k * cols + j;
				fixed *src_ptr = src->data + j;
				fixed w = 0;
				for(uint16_t l = j; l < min(j + BLOCK_SIZE, cols); l++) {
					w = F_ADD(w, F_MUL(*filter_ptr, *src_ptr));
					filter_ptr++;
					src_ptr++;
				}
				*dest_ptr++ += w;
			}
		}
	}
}


int workload_run(){
	task_init();
	task_compute();
	
	return MAT_GET( b1, 0, 0);  
}


#elif WORKLOAD==SCONV

void task_init() {
	uint16_t rows = MAT_GET_DIM((&mat_a_dense), 0);
	uint16_t cols = MAT_GET_DIM((&mat_a_dense), 1);
	//uint16_t frows = MAT_GET_DIM((&mat_c_dense), 2);
	//uint16_t fcols = MAT_GET_DIM((&mat_c_dense), 3);
	MAT_RESHAPE(b1, 1, rows, cols);
	//MAT_RESHAPE(b2, (rows - frows + 1), (cols - fcols + 1));
	//MAT_RESHAPE(b3, (rows - frows + 1), (cols - fcols + 1));
	mat_t *mat_input_ptr = &mat_a_dense;
	for(uint16_t i = 0; i < rows; i++) {
		for(uint16_t j = 0; j < cols; j++) {
			MAT_SET(b1, MAT_GET(mat_input_ptr, i, j), 0, i, j);
		}
	}
}

void task_compute() {
	// PLACE CODE HERE
	mat_t *src = &mat_a_dense;
	mat_t *dest = b1;
	mat_t *filter = &mat_c_sparse;

	uint16_t rows = MAT_GET_DIM(dest, 0);
	uint16_t cols = MAT_GET_DIM(dest, 1);

	uint16_t frows = filter->sparse.dims[2];
	uint16_t fcols = filter->sparse.dims[3];
	
	// Only a 2D convolution
	uint16_t total_elements = filter->sparse.sizes[0] + 1;
	
	uint16_t fsize = frows * fcols;

	fixed *dest_ptr = dest->data;
	for(uint16_t i = 0; i < rows; i++) {
		for(uint16_t j = 0; j < cols; j++) {
			fixed w = 0;
			uint16_t pos = 0;
			uint16_t idx = filter->sparse.offsets[pos];
			fixed *filter_ptr = filter->data;
			while(pos < total_elements) {
				uint16_t k = idx / fsize;
				uint16_t l = (idx % fsize) / fcols;
				uint16_t m = idx % fcols;
				w = F_ADD(w, F_MUL(MAT_GET(src, k, i + l, j + m), *filter_ptr));
				pos++;
				idx += filter->sparse.offsets[pos];
				filter_ptr++;
			}
			*dest_ptr++ = w;
		}
	}
}

int workload_run(){
	task_init();
	task_compute();
	
	return MAT_GET( b1, 0, 0);  
}

#elif WORKLOAD==SMM

void task_compute() {
	
	uint16_t rows;
	uint16_t cols;
	uint16_t dcols;
	
	mat_t *dest = b1;
	mat_t *bsrc = &mat_a_bsparse;
	mat_t *bfilter = &mat_a_bsparse;
	
	rows = bfilter->sparse.dims[0];
	cols = bfilter->sparse.dims[1];
	dcols = bsrc->sparse.dims[1];
	
	fixed *dest_ptr = dest->data;
	for(uint16_t i = 0; i < rows; i++) {
		for(uint16_t j = 0; j < cols; j++) {
			*dest_ptr++ = 0;
		}
	}

	uint16_t block_cols = bsrc->sparse.dims[1];
	uint16_t filter_offset = 0;
	uint16_t filter_block_offset = 0;
	for(uint16_t i = 0; i < rows; i += BLOCK_SIZE) {
		uint16_t src_offset = 0;
		uint16_t src_block_offset = 0;
		for(uint16_t j = 0, crow = 0; 
			j < cols; j += BLOCK_SIZE, crow += (BLOCK_SIZE + 1) * block_cols) {

			for(uint16_t k = 0, ccol = 0; k < dcols; 
				k += BLOCK_SIZE, ccol += (BLOCK_SIZE + 1)) {

				uint16_t m = filter_block_offset;
				for(; m < filter_block_offset + BLOCK_SIZE; m++) {
					uint16_t row = i + m - filter_block_offset;
					fixed *dest_ptr = MAT_PTR(dest, row, 0);
					uint16_t n = bfilter->sparse.sizes[m] + filter_offset;
					for(; n < bfilter->sparse.sizes[m + 1] + filter_offset; n++) {
						fixed f = bfilter->data[n];
						uint16_t col = bfilter->sparse.offsets[n];

						uint16_t col_idx = crow + ccol + col % BLOCK_SIZE;
						uint16_t p = bsrc->sparse.sizes[col_idx] + src_offset;
						for(; p < bsrc->sparse.sizes[col_idx + 1] + src_offset; p++) {
							fixed w = F_MUL(f, bsrc->data[p]);
							uint16_t dcol = bsrc->sparse.offsets[p];
							fixed *d = dest_ptr + dcol;
							*d = F_ADD(w, *d);
						}
					}
				}

				src_offset += bsrc->sparse.sizes[src_block_offset + BLOCK_SIZE];
				src_block_offset += BLOCK_SIZE + 1;
			}

			filter_offset += bfilter->sparse.sizes[filter_block_offset + BLOCK_SIZE];
			filter_block_offset += BLOCK_SIZE + 1;
		}
	}
}


int workload_run(){
	task_compute();

	return MAT_GET( b1, 0, 0);  
}


#elif WORKLOAD==SMV

void task_init() {
	MAT_RESHAPE(b1, MAT_GET_DIM((&mat_b_dense), 0), 1);
	//MAT_RESHAPE(b2, MAT_GET_DIM((&mat_a_dense), 0), 1);
	//MAT_RESHAPE(b3, MAT_GET_DIM((&mat_a_dense), 0), 1);
	mat_t *mat_input_ptr = &mat_b_dense;
	for(uint16_t i = 0; i < MAT_GET_DIM((&mat_b_dense), 0); 
		i++) {
		MAT_SET(b1, MAT_GET(mat_input_ptr, i, 0), i, 0);
	}
}

void task_compute() {
	// PLACE CODE HERE
	mat_t *src = &mat_b_dense;
	mat_t *dest = b1;
	mat_t *bfilter = &mat_a_bsparse;

	uint16_t rows = MAT_GET_DIM(dest, 0);

	uint16_t block_cols = bfilter->sparse.dims[1];
	uint16_t cols = block_cols * BLOCK_SIZE;
	uint16_t block_offset = 0;
	uint16_t offset = 0;
	for(uint16_t i = 0; i < rows; i += BLOCK_SIZE) {
		for(uint16_t j = 0; j < cols; j += BLOCK_SIZE) {
			fixed *dest_ptr = dest->data + i;
			for(uint16_t k = block_offset; k < block_offset + BLOCK_SIZE; k++) {
				fixed w = 0;
				uint16_t l = bfilter->sparse.sizes[k] + offset;
				for(; l < bfilter->sparse.sizes[k + 1] + offset; l++) {
					fixed *src_ptr = src->data + bfilter->sparse.offsets[l];
					w = F_ADD(w, F_MUL(bfilter->data[l], *src_ptr));
				}
				*dest_ptr++ += w;
			}
			offset += bfilter->sparse.sizes[block_offset + BLOCK_SIZE];
			block_offset += BLOCK_SIZE + 1;
		}
	}

}

int workload_run(){
	task_init();
	task_compute();
	
	return MAT_GET( b1, 0, 0);  
}


#endif


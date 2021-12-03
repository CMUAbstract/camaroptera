#include "main.h"
#include <libfixed/fixed.h>
#include "inputs/conv_filter.h"

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


#if WORKLOAD == DCONV

#include "inputs/dconv_mat.h"

#define S 1
#define OP_WIDTH (WIDTH-F_WIDTH+1)/S
#define OP_HEIGHT (HEIGHT-F_HEIGHT+1)/S

fixed workload_run(){ 
	
	int i, j, ii, jj;
	fixed sum, temp;
	fixed dconv_output[OP_HEIGHT][OP_WIDTH];

	for( i = 0; i < OP_HEIGHT; i++ ){
		for( j = 0; j < OP_WIDTH; j++){
			//index = i*OP_WIDTH + j;
			dconv_output[i][j] = 0;
			}
		}

	for( i = 0; i < OP_HEIGHT; i++ ){
		for( j = 0; j < OP_WIDTH; j++ ){
			sum = 0;		
			for( ii = 0; ii < ( F_HEIGHT < OP_HEIGHT ? F_HEIGHT : OP_HEIGHT ); ii++ ){
				for( jj = 0; jj < ( F_WIDTH < OP_WIDTH ? F_WIDTH : OP_WIDTH ); jj++ ){
					temp = F_MUL( filter[ii][jj], image[i+ii][j+jj] );
					sum = F_ADD( sum, temp);
					}
				}
				//index = i*OP_WIDTH + j;
				dconv_output[i][j] = sum;
			}
		}
	

	fixed retVal = dconv_output[0][0];
	
	return retVal;
}

#elif WORKLOAD == DMM

#include "inputs/dmm_mat.h"

#define BLOCK_SIZE 8
#define CEIL(a,b) ( (a+b-1)/b )
#define PROUND BLOCK_SIZE * CEIL(DIM_P,BLOCK_SIZE)
#define NROUND BLOCK_SIZE * CEIL(DIM_N,BLOCK_SIZE)

fixed workload_run(){ 
	int i, j, k, ii, jj, kk;
	fixed sum, temp;		
	fixed dmm_output[DIM_M][DIM_P];

	for( i = 0; i < DIM_M; i++ ){
		for( j = 0; j < DIM_P; j++ ){
			//index = i*DIM_P + j;
			dmm_output[i][j] = 0;
			}
		}

	for(kk = 0; kk < NROUND; kk += BLOCK_SIZE) {
		for(jj = 0; jj < PROUND; jj += BLOCK_SIZE) {
			for(i = 0; i < DIM_M; i++) {
				for(j = jj; j < ( (jj + BLOCK_SIZE) < DIM_P ? (jj + BLOCK_SIZE) : DIM_P ) ; j++) {
					//index = i*DIM_P + j;
					sum = dmm_output[i][j];
					for(k = kk; k < ( (kk + BLOCK_SIZE) < DIM_N ? (kk + BLOCK_SIZE) : DIM_N ) ; k++) {
						temp = F_MUL(matrix_A[i][k], matrix_B[k][j]);
						sum = F_ADD(sum, temp);
						}
					dmm_output[i][j] = sum;
					}
				}
			}
		}

	fixed retVal = dmm_output[0][0];
	return retVal;
}

#elif WORKLOAD==DMV

#include "inputs/dmv_mat.h"

fixed workload_run(){ 
	
	long int i, j, k, ii, jj, kk, index;
	fixed sum, temp;		
	fixed dmv_op[DIM_M];

	for( i = 0; i < DIM_M; i++ ){
		dmv_op[i] = 0;
	}

	for( i = 0; i < DIM_M; i++ ){
		sum = 0;
		for( j = 0; j < DIM_N; j++ ){
			sum = F_ADD( sum, F_MUL( matA[i][j], vecA[j] ) ); 
		}
		dmv_op[i] = sum;
	}

	fixed retVal = dmv_op[0];
	return retVal;
}

#elif WORKLOAD==SCONV

#include "inputs/sconv_mat.h"

#define S 1

#define OP_WIDTH (WIDTH-F_WIDTH+1)/S
#define OP_HEIGHT (HEIGHT-F_HEIGHT+1)/S

fixed workload_run(){ 
	int i, j, ii, jj, index;
	int row, col, val, row_start, row_end, row_idx;
	fixed sconv_output[OP_HEIGHT][OP_WIDTH];
	fixed temp;
	row = 0;
	row_idx = 1;
	row_start = indptr[row_idx-1];
	row_end = indptr[row_idx];
	
	for( i = 0; i < OP_HEIGHT; i++ ){
		for( j = 0; j < OP_WIDTH; j++){
			//index = i*OP_WIDTH + j;
			sconv_output[i][j] = 0;
			}
		}


	for( i = 0; i < NNZ; i++ ){
		while (i >= row_end){
			row_start = row_end;
			row = row_idx;
			row_end = indptr[++row_idx];
			}
			
		col = indices[i];
		val = data[i];
		for( ii = ( (row + 1 - OP_HEIGHT) < 0 ? 0 : (row + 1 - OP_HEIGHT) ); ii < ( (row + 1) < F_HEIGHT ? (row + 1) : F_HEIGHT ) ; ii++ ){
			for( jj = ( (col + 1 - OP_WIDTH) < 0 ? 0 : (col + 1 - OP_WIDTH) ); jj < ( (col + 1) < F_WIDTH ? (col + 1) : F_WIDTH ); jj++ ){
				index = (row-ii)*OP_WIDTH + (col-jj);
				temp = sconv_output[row-ii][col-jj];
				sconv_output[row-ii][col-jj] = F_ADD( temp, F_MUL(val, filter[ii][jj]) );
				}
			}
		}
	
	fixed retVal = sconv_output[0][0];
	return retVal;
}

#elif WORKLOAD==SMM

#include "inputs/smm_mat.h"

fixed workload_run(){ 
	
	int i, j, idxA, index;
	int startA, startB, endA, endB;
	int rowA, colA, colB;
	fixed valA, valB;
	
	fixed sum, temp;		
	fixed smm_output[DIM_M][DIM_P];
	
	rowA = 0;
	idxA = 1;
	startA = indptrA[idxA-1];
	endA = indptrA[idxA];

	for( i = 0; i < DIM_M; i++ ){
		for( j = 0; j < DIM_P; j++ ){
			//index = i*DIM_P + j;
			smm_output[i][j] = 0;
			}
		}

	for( i = 0; i < NNZA; i++ ){ 
		while (i >= endA){
			startA = endA;
			rowA = idxA;
			endA = indptrA[++idxA];
			}
		
		colA = indicesA[i];
		valA = dataA[i];

		startB = indptrB[colA];
		endB = indptrB[colA+1];
		
		for( j = startB; j < endB; j++ ){
			colB = indicesB[j];
			valB = dataB[j];
			index = rowA*DIM_P + colB;
			temp = smm_output[index];
			smm_output[index] = F_ADD( temp, F_MUL(valA, valB) );
		}
	}
	
	fixed retVal = smm_output[0][0];
	return retVal;
}

#elif WORKLOAD==SMV

#include "inputs/smv_mat.h"

fixed workload_run(){ 
	int i, j, idxA;
	int startA, startB, endA, endB;
	int rowA, colA, colB;
	fixed valA, valB;
	
	fixed sum, temp;		
	fixed smv_op[DIM_M];

	for( i = 0; i < DIM_M; i++ ){
		smv_op[i] = 0;
	}
	
	for( i = 0; i < DIM_M; i++ ){
		startA = indptrA[i];
		endA = indptrA[i+1];
		sum = 0;
		for( j = startA; j < endA; j++ ){
			colA = indicesA[j];
			valA = matA[j];
			sum = F_ADD( sum, F_MUL( valA, vecA[colA] ) );
		}
		smv_op[i] = sum;
	}

	fixed retVal = smv_op[0];
	return retVal;
}

#endif


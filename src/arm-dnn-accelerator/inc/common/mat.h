#define __ro_flash __attribute__((__section__(".rodata")))

#if WORKLOAD==DCONV

#if INPUT_SIZE == 2
#pragma message "Medium"
#include "medium/a_dense.h"
#include "medium/c_dense.h"
#define MAT_SIZE 64
#define TILE_SIZE 5
#elif INPUT_SIZE == 3
#pragma message "Large"
#include "large/a_dense.h"
#include "large/b_dense.h"
#include "large/c_dense.h"
#define MAT_SIZE 128
#define TILE_SIZE 5
#endif

#elif WORKLOAD == DMM

#if INPUT_SIZE == 2
#pragma message "Medium"
#include "medium/a_dense.h"
#define MAT_SIZE 64
#define TILE_SIZE 5
#elif INPUT_SIZE == 3
#pragma message "Large"
#include "large/a_dense.h"
#define MAT_SIZE 128
#define TILE_SIZE 5
#endif

#elif WORKLOAD == DMV

#if INPUT_SIZE == 2
#pragma message "Medium"
#include "medium/a_dense.h"
#include "medium/b_dense.h"
#define MAT_SIZE 64
#define TILE_SIZE 5
#elif INPUT_SIZE == 3
#pragma message "Large"
#include "large/a_dense.h"
#include "large/b_dense.h"
#define MAT_SIZE 128
#define TILE_SIZE 5
#endif

#elif WORKLOAD == SCONV

#if INPUT_SIZE == 2
#pragma message "Medium"
#include "medium/a_dense.h"
#include "medium/c_sparse.h"
#define MAT_SIZE 64
#define TILE_SIZE 5
#elif INPUT_SIZE == 3
#pragma message "Large"
#include "large/a_dense.h"
#include "large/c_sparse.h"
#define MAT_SIZE 128
#define TILE_SIZE 5
#endif

#elif WORKLOAD == SMM

#if INPUT_SIZE == 2
#pragma message "Medium"
#include "medium/a_bsparse.h"
#define MAT_SIZE 64
#define TILE_SIZE 5
#elif INPUT_SIZE == 3
#pragma message "Large"
#include "large/a_bsparse.h"
#define MAT_SIZE 128
#define TILE_SIZE 5
#endif

#elif WORKLOAD == SMV

#if INPUT_SIZE == 2
#pragma message "Medium"
#include "medium/a_bsparse.h"
#include "medium/b_dense.h"
#define MAT_SIZE 64
#define TILE_SIZE 5
#elif INPUT_SIZE == 3
#pragma message "Large"
#include "large/a_bsparse.h"
#include "large/b_dense.h"
#define MAT_SIZE 128
#define TILE_SIZE 5
#endif

#endif

#define MAT_BLOCK_SIZE BLOCK_SIZE
#define MAT_BLOCK_COLS MAT_SIZE / BLOCK_SIZE

#ifdef A_DENSE_H
__ro_flash mat_t mat_a_dense = {
		.dims = {MAT_SIZE, MAT_SIZE}, .strides = {MAT_SIZE, 1}, .len_dims = 2, .data = (fixed *) &a_dense,
};
#endif

#ifdef B_DENSE_H
__ro_flash mat_t mat_b_dense = {
		.dims = {MAT_SIZE, 1}, .strides = {1, 1}, .len_dims = 2, .data = b_dense,
};
#endif

#ifdef C_DENSE_H
__ro_flash mat_t mat_c_dense = {
		.dims = {1, 1, TILE_SIZE, TILE_SIZE},
		.len_dims = 4,
		.strides = {TILE_SIZE * TILE_SIZE, TILE_SIZE * TILE_SIZE, TILE_SIZE, 1},
		.data = (fixed *) &c_dense,
};
#endif

#ifdef A_SPARSE_H
__ro_flash mat_t mat_a_sparse = {.dims = {A_SPARSE_LEN},
								.strides = {1},
								.len_dims = 1,
								.data = a_sparse,
								.sparse = {.dims = {MAT_SIZE, MAT_SIZE},
											 .len_dims = 2,
											 .offsets = a_sparse_offsets,
											 .sizes = a_sparse_sizes}};
#endif

#ifdef A_BSPARSE_H
__ro_flash mat_t mat_a_bsparse = {.dims = {A_BSPARSE_LEN},
								.strides = {1},
								.len_dims = 1,
								.data = a_bsparse,
								.sparse = {.dims = {MAT_BLOCK_SIZE, MAT_BLOCK_COLS},
											 .len_dims = 2,
											 .offsets = a_bsparse_offsets,
											 .sizes = a_bsparse_sizes}};
#endif

#ifdef B_SPARSE_H
__ro_flash mat_t mat_b_sparse = {.dims = {B_SPARSE_LEN},
								.strides = {1},
								.len_dims = 1,
								.data = b_sparse,
								.sparse = {.dims = {MAT_SIZE, 1},
											 .len_dims = 2,
											 .offsets = b_sparse_offsets,
											 .sizes = b_sparse_sizes}};
#endif

#ifdef B_BSPARSE_H
__ro_flash mat_t mat_b_bsparse = {.dims = {B_BSPARSE_LEN},
								.strides = {1},
								.len_dims = 1,
								.data = b_bsparse,
								.sparse = {.dims = {MAT_BLOCK_SIZE, MAT_BLOCK_COLS},
											 .len_dims = 2,
											 .offsets = b_bsparse_offsets,
											 .sizes = b_bsparse_sizes}};
#endif

#ifdef C_SPARSE_H
__ro_flash mat_t mat_c_sparse = {.dims = {C_SPARSE_LEN},
								.len_dims = 1,
								.strides = {1},
								.data = c_sparse,
								.sparse = {.dims = {1, 1, TILE_SIZE, TILE_SIZE},
											 .len_dims = 4,
											 .sizes = c_sparse_sizes,
											 .offsets = c_sparse_offsets}};
#endif


#if INPUT_SIZE >= 3
fixed buf[MAT_SIZE * MAT_SIZE];
#else
fixed buf[MAT_SIZE * MAT_SIZE];
#endif
 mat_t buf1 = {.data = buf};
 mat_t *b1 = &buf1;

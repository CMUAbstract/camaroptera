#ifndef B_SPARSE_H
#define B_SPARSE_H
#include <libfemto/femto.h>
#include <libfixed/fixed.h>

#define B_SPARSE_LEN 11

__nvram fixed b_sparse[11] = {F_LIT(2),  F_LIT(4),  F_LIT(2),  F_LIT(3),
							  F_LIT(-5), F_LIT(-2), F_LIT(-5), F_LIT(-2),
							  F_LIT(2),  F_LIT(2),  F_LIT(-2)};

__nvram uint16_t b_sparse_offsets[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

__nvram uint16_t b_sparse_sizes[17] = {0, 1, 2, 3, 3, 4,  5,  6, 7,
									   8, 8, 9, 9, 9, 10, 10, 11};

#endif
#ifndef C_SPARSE_H
#define C_SPARSE_H
#include <libfemto/femto.h>
#include <libfixed/fixed.h>

#define C_SPARSE_LEN 21

__nvram fixed c_sparse[21] = {
	F_LIT(-4), F_LIT(4),  F_LIT(3),  F_LIT(3), F_LIT(-5), F_LIT(-3), F_LIT(3),
	F_LIT(-5), F_LIT(2),  F_LIT(-4), F_LIT(4), F_LIT(-3), F_LIT(-4), F_LIT(-3),
	F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(2), F_LIT(-4), F_LIT(-5), F_LIT(-4)};

__nvram uint16_t c_sparse_offsets[21] = {0, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1,
										 1, 1, 1, 1, 1, 2, 1, 1, 1, 1};

__nvram uint16_t c_sparse_sizes[1] = {21};

#endif
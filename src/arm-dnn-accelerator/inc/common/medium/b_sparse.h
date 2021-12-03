#ifndef B_SPARSE_H
#define B_SPARSE_H
#include <libfemto/femto.h>
#include <libfixed/fixed.h>

#define B_SPARSE_LEN 43

__nvram fixed b_sparse[43] = {
	F_LIT(-3), F_LIT(-2), F_LIT(-5), F_LIT(4),  F_LIT(3),  F_LIT(-3), F_LIT(-2),
	F_LIT(-2), F_LIT(4),  F_LIT(2),  F_LIT(-3), F_LIT(4),  F_LIT(-4), F_LIT(-5),
	F_LIT(-3), F_LIT(3),  F_LIT(-5), F_LIT(-4), F_LIT(3),  F_LIT(2),  F_LIT(-2),
	F_LIT(2),  F_LIT(2),  F_LIT(-4), F_LIT(3),  F_LIT(4),  F_LIT(-4), F_LIT(-5),
	F_LIT(3),  F_LIT(3),  F_LIT(2),  F_LIT(3),  F_LIT(-3), F_LIT(-2), F_LIT(-2),
	F_LIT(-5), F_LIT(2),  F_LIT(-4), F_LIT(-4), F_LIT(-2), F_LIT(-4), F_LIT(2),
	F_LIT(3)};

__nvram uint16_t b_sparse_offsets[43] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

__nvram uint16_t b_sparse_sizes[65] = {
	0,  0,  0,  1,  1,  2,  2,  3,  4,  5,  6,  7,  8,  8,  8,  9,  10,
	11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 25, 26, 27, 28, 28, 29, 30, 30, 30, 31, 32, 32,
	33, 33, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42, 43, 43};

#endif
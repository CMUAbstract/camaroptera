#ifndef A_BSPARSE_H
#define A_BSPARSE_H
#include <libfemto/femto.h>
#include <libfixed/fixed.h>

#if BLOCK_SIZE == 16

#define A_BSPARSE_LEN 179

__nvram fixed a_bsparse[179] = {
	F_LIT(-2), F_LIT(-3), F_LIT(-3), F_LIT(-2), F_LIT(-5), F_LIT(-2), F_LIT(3),
	F_LIT(3),  F_LIT(-3), F_LIT(3),  F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(-3),
	F_LIT(3),  F_LIT(-2), F_LIT(3),  F_LIT(-5), F_LIT(-3), F_LIT(-5), F_LIT(-3),
	F_LIT(-4), F_LIT(-5), F_LIT(2),  F_LIT(-5), F_LIT(-5), F_LIT(-4), F_LIT(-4),
	F_LIT(-5), F_LIT(-2), F_LIT(3),  F_LIT(2),  F_LIT(4),  F_LIT(2),  F_LIT(-5),
	F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(-3), F_LIT(2),  F_LIT(3),  F_LIT(-5),
	F_LIT(-3), F_LIT(-3), F_LIT(-4), F_LIT(-2), F_LIT(-4), F_LIT(-3), F_LIT(-4),
	F_LIT(3),  F_LIT(3),  F_LIT(-2), F_LIT(2),  F_LIT(3),  F_LIT(-4), F_LIT(2),
	F_LIT(3),  F_LIT(-2), F_LIT(4),  F_LIT(-4), F_LIT(-5), F_LIT(-4), F_LIT(-3),
	F_LIT(-3), F_LIT(-4), F_LIT(3),  F_LIT(-3), F_LIT(4),  F_LIT(2),  F_LIT(4),
	F_LIT(-3), F_LIT(-5), F_LIT(2),  F_LIT(2),  F_LIT(-4), F_LIT(4),  F_LIT(-5),
	F_LIT(3),  F_LIT(4),  F_LIT(-2), F_LIT(-5), F_LIT(-3), F_LIT(-3), F_LIT(-4),
	F_LIT(-3), F_LIT(-5), F_LIT(3),  F_LIT(-4), F_LIT(2),  F_LIT(-3), F_LIT(-3),
	F_LIT(2),  F_LIT(-5), F_LIT(-3), F_LIT(2),  F_LIT(4),  F_LIT(-4), F_LIT(-2),
	F_LIT(2),  F_LIT(-4), F_LIT(-5), F_LIT(3),  F_LIT(-5), F_LIT(4),  F_LIT(4),
	F_LIT(3),  F_LIT(2),  F_LIT(4),  F_LIT(3),  F_LIT(-3), F_LIT(-4), F_LIT(4),
	F_LIT(4),  F_LIT(-4), F_LIT(-4), F_LIT(-4), F_LIT(3),  F_LIT(-2), F_LIT(-4),
	F_LIT(4),  F_LIT(4),  F_LIT(3),  F_LIT(-5), F_LIT(-5), F_LIT(4),  F_LIT(-5),
	F_LIT(-2), F_LIT(-4), F_LIT(4),  F_LIT(3),  F_LIT(-2), F_LIT(-5), F_LIT(-5),
	F_LIT(3),  F_LIT(3),  F_LIT(3),  F_LIT(-5), F_LIT(-2), F_LIT(-5), F_LIT(-3),
	F_LIT(4),  F_LIT(3),  F_LIT(-5), F_LIT(3),  F_LIT(4),  F_LIT(2),  F_LIT(-4),
	F_LIT(4),  F_LIT(-5), F_LIT(3),  F_LIT(2),  F_LIT(-2), F_LIT(-5), F_LIT(2),
	F_LIT(-3), F_LIT(2),  F_LIT(4),  F_LIT(-3), F_LIT(2),  F_LIT(2),  F_LIT(-3),
	F_LIT(4),  F_LIT(2),  F_LIT(-3), F_LIT(-2), F_LIT(-2), F_LIT(-3), F_LIT(-3),
	F_LIT(-4), F_LIT(2),  F_LIT(-2), F_LIT(2),  F_LIT(2),  F_LIT(-3), F_LIT(-3),
	F_LIT(4),  F_LIT(3),  F_LIT(4),  F_LIT(-3)};

__nvram uint16_t a_bsparse_offsets[179] = {
	0,  2,  3,  7,  8,  9,  10, 11, 12, 13, 14, 15, 1,  3,  4,  5,  6,  8,
	9,  10, 11, 12, 13, 14, 0,  2,  3,  4,  5,  6,  7,  9,  10, 11, 12, 14,
	15, 0,  2,  3,  4,  5,  6,  7,  9,  10, 12, 13, 14, 1,  2,  3,  4,  7,
	8,  9,  10, 11, 12, 13, 15, 0,  3,  4,  5,  6,  7,  11, 14, 15, 0,  1,
	3,  5,  8,  9,  10, 11, 12, 14, 0,  1,  2,  3,  4,  5,  6,  7,  9,  10,
	13, 15, 1,  3,  4,  5,  7,  8,  9,  10, 12, 13, 15, 0,  2,  3,  5,  6,
	9,  10, 11, 12, 13, 14, 15, 1,  2,  3,  4,  5,  7,  8,  10, 11, 12, 14,
	15, 0,  2,  3,  5,  6,  7,  8,  9,  10, 12, 13, 0,  1,  2,  5,  6,  7,
	8,  9,  10, 11, 12, 13, 14, 1,  2,  4,  7,  10, 11, 12, 15, 0,  1,  3,
	6,  7,  10, 11, 12, 13, 14, 15, 0,  2,  3,  4,  5,  6,  7,  13, 15};

__nvram uint16_t a_bsparse_sizes[17] = {
	0, 12, 24, 37, 49, 61, 70, 80, 92, 103, 115, 127, 138, 151, 159, 170, 179};

#elif BLOCK_SIZE == 32

#define A_BSPARSE_LEN 179

__nvram fixed a_bsparse[179] = {
	F_LIT(-2), F_LIT(-3), F_LIT(-3), F_LIT(-2), F_LIT(-5), F_LIT(-2), F_LIT(3),
	F_LIT(3),  F_LIT(-3), F_LIT(3),  F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(-3),
	F_LIT(3),  F_LIT(-2), F_LIT(3),  F_LIT(-5), F_LIT(-3), F_LIT(-5), F_LIT(-3),
	F_LIT(-4), F_LIT(-5), F_LIT(2),  F_LIT(-5), F_LIT(-5), F_LIT(-4), F_LIT(-4),
	F_LIT(-5), F_LIT(-2), F_LIT(3),  F_LIT(2),  F_LIT(4),  F_LIT(2),  F_LIT(-5),
	F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(-3), F_LIT(2),  F_LIT(3),  F_LIT(-5),
	F_LIT(-3), F_LIT(-3), F_LIT(-4), F_LIT(-2), F_LIT(-4), F_LIT(-3), F_LIT(-4),
	F_LIT(3),  F_LIT(3),  F_LIT(-2), F_LIT(2),  F_LIT(3),  F_LIT(-4), F_LIT(2),
	F_LIT(3),  F_LIT(-2), F_LIT(4),  F_LIT(-4), F_LIT(-5), F_LIT(-4), F_LIT(-3),
	F_LIT(-3), F_LIT(-4), F_LIT(3),  F_LIT(-3), F_LIT(4),  F_LIT(2),  F_LIT(4),
	F_LIT(-3), F_LIT(-5), F_LIT(2),  F_LIT(2),  F_LIT(-4), F_LIT(4),  F_LIT(-5),
	F_LIT(3),  F_LIT(4),  F_LIT(-2), F_LIT(-5), F_LIT(-3), F_LIT(-3), F_LIT(-4),
	F_LIT(-3), F_LIT(-5), F_LIT(3),  F_LIT(-4), F_LIT(2),  F_LIT(-3), F_LIT(-3),
	F_LIT(2),  F_LIT(-5), F_LIT(-3), F_LIT(2),  F_LIT(4),  F_LIT(-4), F_LIT(-2),
	F_LIT(2),  F_LIT(-4), F_LIT(-5), F_LIT(3),  F_LIT(-5), F_LIT(4),  F_LIT(4),
	F_LIT(3),  F_LIT(2),  F_LIT(4),  F_LIT(3),  F_LIT(-3), F_LIT(-4), F_LIT(4),
	F_LIT(4),  F_LIT(-4), F_LIT(-4), F_LIT(-4), F_LIT(3),  F_LIT(-2), F_LIT(-4),
	F_LIT(4),  F_LIT(4),  F_LIT(3),  F_LIT(-5), F_LIT(-5), F_LIT(4),  F_LIT(-5),
	F_LIT(-2), F_LIT(-4), F_LIT(4),  F_LIT(3),  F_LIT(-2), F_LIT(-5), F_LIT(-5),
	F_LIT(3),  F_LIT(3),  F_LIT(3),  F_LIT(-5), F_LIT(-2), F_LIT(-5), F_LIT(-3),
	F_LIT(4),  F_LIT(3),  F_LIT(-5), F_LIT(3),  F_LIT(4),  F_LIT(2),  F_LIT(-4),
	F_LIT(4),  F_LIT(-5), F_LIT(3),  F_LIT(2),  F_LIT(-2), F_LIT(-5), F_LIT(2),
	F_LIT(-3), F_LIT(2),  F_LIT(4),  F_LIT(-3), F_LIT(2),  F_LIT(2),  F_LIT(-3),
	F_LIT(4),  F_LIT(2),  F_LIT(-3), F_LIT(-2), F_LIT(-2), F_LIT(-3), F_LIT(-3),
	F_LIT(-4), F_LIT(2),  F_LIT(-2), F_LIT(2),  F_LIT(2),  F_LIT(-3), F_LIT(-3),
	F_LIT(4),  F_LIT(3),  F_LIT(4),  F_LIT(-3)};

__nvram uint16_t a_bsparse_offsets[179] = {
	0,  2,  3,  7,  8,  9,  10, 11, 12, 13, 14, 15, 1,  3,  4,  5,  6,  8,
	9,  10, 11, 12, 13, 14, 0,  2,  3,  4,  5,  6,  7,  9,  10, 11, 12, 14,
	15, 0,  2,  3,  4,  5,  6,  7,  9,  10, 12, 13, 14, 1,  2,  3,  4,  7,
	8,  9,  10, 11, 12, 13, 15, 0,  3,  4,  5,  6,  7,  11, 14, 15, 0,  1,
	3,  5,  8,  9,  10, 11, 12, 14, 0,  1,  2,  3,  4,  5,  6,  7,  9,  10,
	13, 15, 1,  3,  4,  5,  7,  8,  9,  10, 12, 13, 15, 0,  2,  3,  5,  6,
	9,  10, 11, 12, 13, 14, 15, 1,  2,  3,  4,  5,  7,  8,  10, 11, 12, 14,
	15, 0,  2,  3,  5,  6,  7,  8,  9,  10, 12, 13, 0,  1,  2,  5,  6,  7,
	8,  9,  10, 11, 12, 13, 14, 1,  2,  4,  7,  10, 11, 12, 15, 0,  1,  3,
	6,  7,  10, 11, 12, 13, 14, 15, 0,  2,  3,  4,  5,  6,  7,  13, 15};

__nvram uint16_t a_bsparse_sizes[17] = {
	0, 12, 24, 37, 49, 61, 70, 80, 92, 103, 115, 127, 138, 151, 159, 170, 179};

#elif BLOCK_SIZE == 64

#define A_BSPARSE_LEN 179

__nvram fixed a_bsparse[179] = {
	F_LIT(-2), F_LIT(-3), F_LIT(-3), F_LIT(-2), F_LIT(-5), F_LIT(-2), F_LIT(3),
	F_LIT(3),  F_LIT(-3), F_LIT(3),  F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(-3),
	F_LIT(3),  F_LIT(-2), F_LIT(3),  F_LIT(-5), F_LIT(-3), F_LIT(-5), F_LIT(-3),
	F_LIT(-4), F_LIT(-5), F_LIT(2),  F_LIT(-5), F_LIT(-5), F_LIT(-4), F_LIT(-4),
	F_LIT(-5), F_LIT(-2), F_LIT(3),  F_LIT(2),  F_LIT(4),  F_LIT(2),  F_LIT(-5),
	F_LIT(-4), F_LIT(-5), F_LIT(4),  F_LIT(-3), F_LIT(2),  F_LIT(3),  F_LIT(-5),
	F_LIT(-3), F_LIT(-3), F_LIT(-4), F_LIT(-2), F_LIT(-4), F_LIT(-3), F_LIT(-4),
	F_LIT(3),  F_LIT(3),  F_LIT(-2), F_LIT(2),  F_LIT(3),  F_LIT(-4), F_LIT(2),
	F_LIT(3),  F_LIT(-2), F_LIT(4),  F_LIT(-4), F_LIT(-5), F_LIT(-4), F_LIT(-3),
	F_LIT(-3), F_LIT(-4), F_LIT(3),  F_LIT(-3), F_LIT(4),  F_LIT(2),  F_LIT(4),
	F_LIT(-3), F_LIT(-5), F_LIT(2),  F_LIT(2),  F_LIT(-4), F_LIT(4),  F_LIT(-5),
	F_LIT(3),  F_LIT(4),  F_LIT(-2), F_LIT(-5), F_LIT(-3), F_LIT(-3), F_LIT(-4),
	F_LIT(-3), F_LIT(-5), F_LIT(3),  F_LIT(-4), F_LIT(2),  F_LIT(-3), F_LIT(-3),
	F_LIT(2),  F_LIT(-5), F_LIT(-3), F_LIT(2),  F_LIT(4),  F_LIT(-4), F_LIT(-2),
	F_LIT(2),  F_LIT(-4), F_LIT(-5), F_LIT(3),  F_LIT(-5), F_LIT(4),  F_LIT(4),
	F_LIT(3),  F_LIT(2),  F_LIT(4),  F_LIT(3),  F_LIT(-3), F_LIT(-4), F_LIT(4),
	F_LIT(4),  F_LIT(-4), F_LIT(-4), F_LIT(-4), F_LIT(3),  F_LIT(-2), F_LIT(-4),
	F_LIT(4),  F_LIT(4),  F_LIT(3),  F_LIT(-5), F_LIT(-5), F_LIT(4),  F_LIT(-5),
	F_LIT(-2), F_LIT(-4), F_LIT(4),  F_LIT(3),  F_LIT(-2), F_LIT(-5), F_LIT(-5),
	F_LIT(3),  F_LIT(3),  F_LIT(3),  F_LIT(-5), F_LIT(-2), F_LIT(-5), F_LIT(-3),
	F_LIT(4),  F_LIT(3),  F_LIT(-5), F_LIT(3),  F_LIT(4),  F_LIT(2),  F_LIT(-4),
	F_LIT(4),  F_LIT(-5), F_LIT(3),  F_LIT(2),  F_LIT(-2), F_LIT(-5), F_LIT(2),
	F_LIT(-3), F_LIT(2),  F_LIT(4),  F_LIT(-3), F_LIT(2),  F_LIT(2),  F_LIT(-3),
	F_LIT(4),  F_LIT(2),  F_LIT(-3), F_LIT(-2), F_LIT(-2), F_LIT(-3), F_LIT(-3),
	F_LIT(-4), F_LIT(2),  F_LIT(-2), F_LIT(2),  F_LIT(2),  F_LIT(-3), F_LIT(-3),
	F_LIT(4),  F_LIT(3),  F_LIT(4),  F_LIT(-3)};

__nvram uint16_t a_bsparse_offsets[179] = {
	0,  2,  3,  7,  8,  9,  10, 11, 12, 13, 14, 15, 1,  3,  4,  5,  6,  8,
	9,  10, 11, 12, 13, 14, 0,  2,  3,  4,  5,  6,  7,  9,  10, 11, 12, 14,
	15, 0,  2,  3,  4,  5,  6,  7,  9,  10, 12, 13, 14, 1,  2,  3,  4,  7,
	8,  9,  10, 11, 12, 13, 15, 0,  3,  4,  5,  6,  7,  11, 14, 15, 0,  1,
	3,  5,  8,  9,  10, 11, 12, 14, 0,  1,  2,  3,  4,  5,  6,  7,  9,  10,
	13, 15, 1,  3,  4,  5,  7,  8,  9,  10, 12, 13, 15, 0,  2,  3,  5,  6,
	9,  10, 11, 12, 13, 14, 15, 1,  2,  3,  4,  5,  7,  8,  10, 11, 12, 14,
	15, 0,  2,  3,  5,  6,  7,  8,  9,  10, 12, 13, 0,  1,  2,  5,  6,  7,
	8,  9,  10, 11, 12, 13, 14, 1,  2,  4,  7,  10, 11, 12, 15, 0,  1,  3,
	6,  7,  10, 11, 12, 13, 14, 15, 0,  2,  3,  4,  5,  6,  7,  13, 15};

__nvram uint16_t a_bsparse_sizes[17] = {
	0, 12, 24, 37, 49, 61, 70, 80, 92, 103, 115, 127, 138, 151, 159, 170, 179};

#endif

#endif
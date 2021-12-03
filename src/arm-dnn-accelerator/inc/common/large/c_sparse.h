#ifndef C_SPARSE_H
#define C_SPARSE_H
#include <libfixed/fixed.h>

#define C_SPARSE_LEN 15

#define __ro_flash __attribute__((section(".rodata")))

__ro_flash fixed c_sparse[15] = {F_LIT(-3), F_LIT(-5), F_LIT(2),  F_LIT(-4),
                              F_LIT(2),  F_LIT(-2), F_LIT(-2), F_LIT(-3),
                              F_LIT(2),  F_LIT(-4), F_LIT(-5), F_LIT(-5),
                              F_LIT(4),  F_LIT(4),  F_LIT(4)};

__ro_flash uint16_t c_sparse_offsets[15] = {1, 1, 1, 1, 1, 2, 1, 4,
                                         1, 3, 2, 1, 2, 2, 1};

__ro_flash uint16_t c_sparse_sizes[1] = {15};

#endif

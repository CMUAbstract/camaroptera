#ifndef C_DENSE_H
#define C_DENSE_H
#include <libfixed/fixed.h>

#define __ro_flash __attribute__((section(".rodata")))

__ro_flash fixed c_dense[5][5] = {
	{F_LIT(0), F_LIT(-5), F_LIT(-2), F_LIT(-5), F_LIT(0)},
	{F_LIT(-4), F_LIT(-5), F_LIT(3), F_LIT(3), F_LIT(-4)},
	{F_LIT(-2), F_LIT(-5), F_LIT(3), F_LIT(-5), F_LIT(-4)},
	{F_LIT(-4), F_LIT(-5), F_LIT(0), F_LIT(-4), F_LIT(-2)},
	{F_LIT(-2), F_LIT(2), F_LIT(0), F_LIT(-5), F_LIT(-4)}};

#endif

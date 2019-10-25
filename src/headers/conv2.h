#ifndef CONV2_H
#define CONV2_H
#include <libfixed/fixed.h>
#include <libdnn/mem.h>

#define CONV2_WM_LEN 9

__ro_hifram fixed conv2_wm[9] = {F_LIT(0.4375),F_LIT(0.21875), F_LIT(0.375), F_LIT(0.34375), F_LIT(0.40625), F_LIT(0.5), F_LIT(0.5625),F_LIT(-0.1875), F_LIT(-0.1875)};

__ro_hifram fixed conv2_wm_offsets[9] = {493,476, 2, 3, 1, 1, 1,233, 1};

__ro_hifram fixed conv2_wm_sizes[100] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

__ro_hifram fixed conv2_b[100] = {F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(-0.03125), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(-0.03125), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0), F_LIT(0.0)};

#endif
#ifndef CONV1_H
#define CONV1_H
#include <libfixed/fixed.h>
#include "lenet.h"

#define CONV1_WMH_LEN 68

__ro_hifram fixed conv1_wmh[68] = {F_LIT(0.4375), F_LIT(0.34375), F_LIT(0.65625), F_LIT(0.46875),F_LIT(0.53125), F_LIT(0.4375), F_LIT(0.625),F_LIT(0.375), F_LIT(0.625), F_LIT(0.5625),F_LIT(0.75), F_LIT(0.59375),F_LIT(0.78125), F_LIT(0.46875),F_LIT(0.3125), F_LIT(0.5), F_LIT(0.5625), F_LIT(0.5),F_LIT(0.5625), F_LIT(0.4375), F_LIT(0.34375), F_LIT(0.53125),F_LIT(0.8125), F_LIT(0.5), F_LIT(-0.75),F_LIT(0.34375), F_LIT(-0.375), F_LIT(0.4375), F_LIT(0.6875),F_LIT(0.59375), F_LIT(-0.5625), F_LIT(-0.40625),F_LIT(0.34375), F_LIT(0.4375), F_LIT(0.3125), F_LIT(0.71875),F_LIT(-0.3125), F_LIT(-0.4375), F_LIT(0.75),F_LIT(0.34375), F_LIT(0.5625), F_LIT(0.5), F_LIT(0.4375),F_LIT(0.34375), F_LIT(0.71875), F_LIT(0.4375),F_LIT(0.46875), F_LIT(-0.375), F_LIT(0.46875), F_LIT(0.5625),F_LIT(0.875), F_LIT(-0.34375),F_LIT(0.6875), F_LIT(0.34375), F_LIT(0.46875), F_LIT(0.3125),F_LIT(0.6875), F_LIT(0.75), F_LIT(1.03125), F_LIT(0.90625), F_LIT(0.90625),F_LIT(0.3125), F_LIT(0.8125), F_LIT(-0.375),F_LIT(0.75), F_LIT(0.6875), F_LIT(0.78125), F_LIT(0.65625)};

__ro_hifram fixed conv1_wmh_offsets[68] = {0, 1, 1, 2,1, 2, 1,0, 1, 1,2, 2,0, 1,0, 1, 1, 1,0, 1, 2, 1,1, 1, 2,0, 1, 2, 1,0, 2, 1,0, 1, 2, 1,1, 2, 1,0, 1, 1, 1,0, 1, 1,0, 2, 1, 1,2, 1,0, 1, 1, 1,0, 1, 1, 1, 1,1, 1, 2,1, 1, 1, 1};

__ro_hifram fixed conv1_wmh_sizes[20] = {4,3,3,2,2,4,4,3,4,3,4,3,4,3,4,2,4,5,3,4};

#define CONV1_WMV_LEN 51

__ro_hifram fixed conv1_wmv[51] = {F_LIT(0.65625), F_LIT(0.625),F_LIT(0.84375), F_LIT(-0.40625),F_LIT(0.3125), F_LIT(0.71875), F_LIT(-0.5625),F_LIT(-0.59375), F_LIT(0.40625), F_LIT(0.625),F_LIT(-0.4375), F_LIT(0.53125), F_LIT(0.65625),F_LIT(-0.34375), F_LIT(0.875),F_LIT(0.28125), F_LIT(0.90625),F_LIT(1.5), F_LIT(2.25), F_LIT(0.53125),F_LIT(0.96875),F_LIT(0.71875), F_LIT(0.65625),F_LIT(0.46875), F_LIT(0.84375),F_LIT(-0.6875), F_LIT(0.6875),F_LIT(-0.4375), F_LIT(-0.3125), F_LIT(-0.375), F_LIT(0.71875),F_LIT(0.625), F_LIT(0.65625),F_LIT(0.375), F_LIT(0.78125),F_LIT(-0.53125), F_LIT(0.375), F_LIT(0.65625),F_LIT(0.65625), F_LIT(0.4375), F_LIT(-0.5),F_LIT(0.90625), F_LIT(-0.4375), F_LIT(-0.75), F_LIT(-0.3125),F_LIT(-0.34375), F_LIT(-0.40625), F_LIT(0.5625), F_LIT(0.59375),F_LIT(-0.15625), F_LIT(1.125)};

__ro_hifram fixed conv1_wmv_offsets[51] = {2, 1,1, 2,2, 1, 1,0, 1, 2,1, 2, 1,1, 2,2, 1,2, 1, 1,0,3, 1,0, 4,1, 1,1, 1, 1, 1,3, 1,1, 1,0, 3, 1,1, 1, 1,0, 2, 1, 1,0, 1, 2, 1,2, 1};

__ro_hifram fixed conv1_wmv_sizes[20] = {2,2,3,3,3,2,2,3,1,2,2,2,4,2,2,3,3,4,4,2};

#define CONV1_WMD_LEN 3

__ro_hifram fixed conv1_wmd[3] = {F_LIT(-2.0625),F_LIT(-0.78125),F_LIT(0.59375)};

__ro_hifram fixed conv1_wmd_offsets[3] = {0,0,0};

__ro_hifram fixed conv1_wmd_sizes[20] = {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1};

__ro_hifram fixed conv1_b[20] = {F_LIT(0.0), F_LIT(-0.03125), F_LIT(-0.0625), F_LIT(0.0), F_LIT(0.0), F_LIT(-0.0625), F_LIT(-0.125), F_LIT(0.15625), F_LIT(0.0), F_LIT(-0.03125), F_LIT(0.0), F_LIT(0.0), F_LIT(-0.0625), F_LIT(-0.0625), F_LIT(0.0), F_LIT(0.0), F_LIT(-0.0625), F_LIT(0.0), F_LIT(0.0), F_LIT(-0.09375)};

#endif

#ifndef REPEAT_H
#define REPEAT_H

#define REPEAT_SETUP uint16_t __unroll_idx = 0;
#define REPEAT_INC __unroll_idx++;
#define REPEAT_1(max, code) if(__unroll_idx < max) { code } REPEAT_INC
#define REPEAT_2(max, code) REPEAT_1(max, code) REPEAT_1(max, code)
#define REPEAT_4(max, code) REPEAT_2(max, code) REPEAT_2(max, code)
#define REPEAT_8(max, code) REPEAT_4(max, code) REPEAT_4(max, code)
#define REPEAT_16(max, code) REPEAT_8(max, code) REPEAT_8(max, code)
#define REPEAT_32(max, code) REPEAT_16(max, code) REPEAT_16(max, code)
#define REPEAT_64(max, code) REPEAT_32(max, code) REPEAT_32(max, code)

#define REPEAT(unroll, max, code) { REPEAT_SETUP REPEAT_##unroll(max, code) }

#endif
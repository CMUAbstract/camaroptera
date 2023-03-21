#define __ro_flash __attribute__((__section__(".rodata")))

#include <stdint.h>

void task_finish();
void task_exit();
void run_DNN();

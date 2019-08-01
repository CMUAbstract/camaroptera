//#include "headers_30x40/input-neg.h"
#include <libalpaca/alpaca.h>

extern int camaroptera_main();
extern uint8_t camaroptera_next_task(uint8_t current_task);

extern TASK_DEC(task_init);
void task_init();
void task_compute();
void task_finish();
void task_exit();



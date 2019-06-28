#include <msp430.h>
#include <stdlib.h>
#include <string.h>

#include <libio/console.h>
#include <libmspbuiltins/builtins.h>
#include <libmsp/mem.h>
#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <libalpaca/alpaca.h>
#include <libfixed/fixed.h>
#include <libmat/mat.h>

#include <libdnn/misc.h>
#include <libdnn/mem.h>
#include <libdnn/state.h>
#include <libdnn/buffer.h>
#include <libdnn/nn.h>
#include <libdnn/nonlinear.h>
#include <libdnn/linalg.h>

#include "headers/conv1.h"
#include "headers/conv2.h"
#include "headers/fc1.h"
#include "headers/fc2.h"
//#include "headers/input.h"

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////Alapaca Shim///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define MEM_SIZE 0x400
__ro_hifram uint8_t *data_src[MEM_SIZE];
__ro_hifram uint8_t *data_dest[MEM_SIZE];
__ro_hifram unsigned int data_size[MEM_SIZE];
void clear_isDirty() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Tasks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void task_init();
void task_compute();
void task_finish();
void task_exit();

TASK(1, task_init);
TASK(2, task_compute);
TASK(3, task_finish);
TASK(4, task_exit);

ENTRY_TASK(task_init)
INIT_FUNC(init)


TOOLCHAINS = gcc

export BOARD = launchpad
export DEVICE = msp430fr5994

EXEC = bird

OBJECTS = main.o camaroptera-dnn.o
OBJECTS = main.o 

DEPS += liblora libio libmsp libhimax libfixed libmspmath libmspbuiltins libalpaca libmat libdnn libjpeg

export MAIN_CLOCK_FREQ = 16000000

export CLOCK_FREQ_ACLK = 32768
export CLOCK_FREQ_SMCLK = 4000000
export CLOCK_FREQ_MCLK = $(MAIN_CLOCK_FREQ)

export LIBMSP_CLOCK_SOURCE = DCO
export LIBMSP_DCO_FREQ = $(MAIN_CLOCK_FREQ)

export LIBFIXED_PRECISE = 1
export LIBFIXED_BITWIDTH = 16
export LIBFIXED_FRAC_BITWIDTH = 5

CONSOLE ?= 1
INTERMITTENT ?= 
BACKEND ?= sonic
CONT ?= 1
FIXED_TEST ?=

MAT_BUF_SIZE ?= 0x310
LAYER_BUF_SIZE ?= 0x3000

ifneq ($(CONSOLE),)
export VERBOSE = 1
export LIBMSP_SLEEP = 1
export LIBIO_BACKEND = hwuart
export LIBMSP_UART_IDX = 0
export LIBMSP_UART_PIN_TX = 2.0
export LIBMSP_UART_BAUDRATE = 115200
export LIBMSP_UART_CLOCK = SMCLK
export LIBDNN_CONSOLE = 1
export LIBMAT_CONSOLE = 1
override CFLAGS += -DCONFIG_CONSOLE=1
endif

#override CFLAGS += -s

ifneq ($(INTERMITTENT),)
export LIBDNN_INTERMITTENT = 1
override CFLAGS += -DCONFIG_INTERMITTENT=1
endif

ifneq ($(FIXED_TEST),)
export LIBFIXED_TEST = 1
endif

ifeq ($(BACKEND), tails)
export LIBDNN_LEA = 1
SHARED_DEPS += libdsp libmspdriver
endif

export LIBDNN_BACKEND = $(BACKEND)
export LIBDNN_TILE_SIZE = 128
export LIBDNN_MAT_BUF_SIZE = $(MAT_BUF_SIZE)
export LIBDNN_LAYER_BUF_SIZE = $(LAYER_BUF_SIZE)
override CC_LD_FLAGS += -mlarge -s -DCAMAROPTERA -DTRANSMITTER_ONLY

export CC_LD_FLAGS

export CFLAGS
export LFLAGS
include tools/maker/Makefile


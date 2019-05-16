TOOLCHAINS = gcc

export BOARD = launchpad
export DEVICE = msp430fr5994

EXEC = bird

OBJECTS = main.o jpec.o
#OBJECTS = hog-svm.o

DEPS += liblora libio libmsp libhimax libfixed libmspmath

export MAIN_CLOCK_FREQ = 16000000

export CLOCK_FREQ_ACLK = 32768
export CLOCK_FREQ_SMCLK = $(MAIN_CLOCK_FREQ)
export CLOCK_FREQ_MCLK = $(MAIN_CLOCK_FREQ)

export LIBMSP_CLOCK_SOURCE = DCO
export LIBMSP_DCO_FREQ = $(MAIN_CLOCK_FREQ)

export LIBFIXED_PRECISE = 1

CONSOLE ?= 1
INTERMITTENT ?= 

ifneq ($(CONSOLE),)
export VERBOSE = 1
export LIBMSP_SLEEP = 1
export LIBIO_BACKEND = hwuart
export LIBMSP_UART_IDX = 0
export LIBMSP_UART_PIN_TX = 2.0
export LIBMSP_UART_BAUDRATE = 115200
export LIBMSP_UART_CLOCK = SMCLK
endif

CFLAGS += -mlarge
LFLAGS += -mlarge

ifneq ($(INTERMITTENT),)
override CFLAGS += -DCONFIG_INTERMITTENT=1
endif

export CFLAGS
export LFLAGS
include tools/maker/Makefile


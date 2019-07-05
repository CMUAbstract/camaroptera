TOOLCHAINS = gcc

export BOARD = launchpad
export DEVICE = msp430fr5994

EXEC = bird

OBJECTS = main.o 
#OBJECTS = hog-svm.o

DEPS += liblora libio libmsp libhimax libfixed libmspmath libfixed libmspbuiltins libmat libjpeg

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

CONT ?= 1
FIXED_TEST ?=

ifneq ($(CONSOLE),)
export VERBOSE = 1
export LIBMSP_SLEEP = 1
export LIBIO_BACKEND = hwuart
export LIBMSP_UART_IDX = 0
export LIBMSP_UART_PIN_TX = 2.0
export LIBMSP_UART_BAUDRATE = 115200
export LIBMSP_UART_CLOCK = SMCLK
endif

#override CFLAGS += -s

ifneq ($(FIXED_TEST),)
export LIBFIXED_TEST = 1
endif

ifneq ($(INTERMITTENT),)
override CFLAGS += -DCONFIG_INTERMITTENT=1
endif

override CC_LD_FLAGS += -mlarge -s -DCAMAROPTERA -DTRANSMITTER_ONLY -DOLD_PINS

export CC_LD_FLAGS

export CFLAGS
export LFLAGS
include tools/maker/Makefile


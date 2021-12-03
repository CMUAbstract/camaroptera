LIB = libmsp

OBJECTS = \
	watchdog.o \
	mem.o \
	clock.o \

ifneq ($(LIBMSP_SLEEP_TIMER),)

# In Clang only polling mode is supported (because no
# __bic_SR_register_on_exit. See here for ideas on how to do it:
# https://github.com/rust-lang-nursery/embedded-wg/issues/20#issuecomment-296538976
ifeq ($(TOOLCHAIN),clang)
ifeq ($(LIBMSP_SLEEP),1)
ifeq ($(findstring clean,$(MAKECMDGOALS)),)
$(error Sleep mode in libmsp/sleep.c is not supported with Clang: \
	unset LIBMSP_SLEEP for polling mode)
endif # !clean
endif # LIBMSP_UART_SLEEP != 1
endif # Clang

OBJECTS += sleep.o
endif # LIBMSP_SLEEP_TIMER

ifneq ($(LIBMSP_UART_IDX),)

# In Clang only polling mode is supported (because no
# __bic_SR_register_on_exit, see comment and link above)
ifeq ($(TOOLCHAIN),clang)
ifeq ($(LIBMSP_SLEEP),1)
ifeq ($(findstring clean,$(MAKECMDGOALS)),)
$(error Sleep mode in libmsp/uart.c is not supported with Clang: \
	unset LIBMSP_SLEEP for polling mode)
endif # !clean
endif # LIBMSP_UART_SLEEP != 1
endif # Clang

OBJECTS += uart.o
endif # LIBMSP_UART_IDX

ifneq ($(LIBMSP_TICK_TIMER),)
OBJECTS += tick.o
endif # LIBMSP_TICK

ifneq ($(LIBMSP_TEMP),)
OBJECTS += temp.o
endif # LIBMSP_TEMP

# pseudo-RNG is only supported with hardware multiplier
ifeq ($(HWMULT),1)
OBJECTS += rand.o
endif

override SRC_ROOT = ../../src

override CFLAGS += \
	-I$(SRC_ROOT)/include/libmsp \

include $(MAKER_ROOT)/Makefile.$(TOOLCHAIN)

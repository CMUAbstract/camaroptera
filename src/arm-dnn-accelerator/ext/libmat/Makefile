LIB = libmat

OBJECTS = mat.o

ifeq ($(PLATFORM),RISCV)
DEPS = libfemto libfixed libio
else
DEPS = libfixed libio
endif

override SRC_ROOT = ../../src

override CFLAGS += -I $(SRC_ROOT)/include/$(LIB)

include $(MAKER_ROOT)/Makefile.$(TOOLCHAIN)

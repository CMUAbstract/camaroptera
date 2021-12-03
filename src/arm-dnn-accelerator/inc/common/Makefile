ifneq ($(CONSOLE),)
override CFLAGS += -DCONSOLE=1
endif

ifneq ($(SIMCALL),)
override CFLAGS += -DSIMCALL=1
endif

ifneq ($(TIME),)
override CFLAGS += -DTIME=1
endif

BACKEND ?= sonic

BACKEND_SRC = "\""$(BACKEND)".h\""
override CFLAGS += -DBACKEND=$(BACKEND_SRC) 

BLOCK_SIZE ?= 16
override CFLAGS += -DBLOCK_SIZE=$(BLOCK_SIZE)

INPUT_SIZE ?= 1
override CFLAGS += -DINPUT_SIZE=$(INPUT_SIZE)

VL ?= 16
override CFLAGS += -DVL=$(VL)

override DEPS += libmat libfemto libspike libio libalpaca libair libfast libpcm libfixed

EXEC = $(EXEC_PREFIX)_$(BACKEND)

FULL_NAME ?=
ifeq ($(FULL_NAME), 1)
	EXEC = $(EXEC_PREFIX)_$(BACKEND)_$(BLOCK_SIZE)_$(INPUT_SIZE)_$(VL)
endif

# Target File
TARGET := main

DEPS := libfixed libmat

# Include Libraries:

prefix := /home/harshd/phase-testbeds/drivers/STM32G4xx/

LIB_CMSIS_PRE := $(prefix)/CMSIS
LIB_CMSIS_INC := $(LIB_CMSIS_PRE)/Include/

LIB_CMSIS_DEV_PRE := $(LIB_CMSIS_PRE)/Device/ST/STM32G4xx
LIB_CMSIS_DEV_INC := $(LIB_CMSIS_DEV_PRE)/Include/
LIB_CMSIS_DEV_SRC := $(LIB_CMSIS_DEV_PRE)/Source/Templates

LIB_PERIPHERALS_PRE := $(prefix)/STM32G4xx_HAL_Driver
LIB_PERIPHERALS_INC := $(LIB_PERIPHERALS_PRE)/Inc/
LIB_PERIPHERALS_SRC := $(LIB_PERIPHERALS_PRE)/Src/

LIB_BSP := $(prefix)/BSP/STM32G4xx_Nucleo/

# Compiler commands
CC := arm-none-eabi-gcc
CP := arm-none-eabi-objcopy
LD := arm-none-eabi-ld
AR := arm-none-eabi-ar
DUMP := arm-none-eabi-objdump

# Select microcontroller
CPU := cortex-m4
ARCH := armv7e-m

# Paths to files
SOURCE_PATH := ./src
INCLUDE_PATH := ./inc
UTILS_PATH := ./utils
TOOLS_PATH := ./tools
ARCHIVE_PATH := ./archive
OBJ_PATH := ./obj
BUILD_PATH := ./bld
EXT_DIR := ./ext

# Just get the OS automatically
# shell cat /proc/version outputs OS and version info
# the filter looks for arch linux, cygwin or ubuntu
# the patsubst is becuase Arch's unique word involves version info and that
# makes for a long, brittle name, so we change that word to ARCH if we find it
OS := $(patsubst %-ARCH,ARCH,$(filter %-ARCH CYGWIN_NT-10.0 Ubuntu, $(shell cat /proc/version)))
UPLOAD_PATH_CYGWIN_NT-10.0 := /cygdrive/d
UPLOAD_PATH_UBUNTU := /media/harshd/NOD_G474RE
UPLOAD_PATH_ARCH := /dev/ttyACM0
UPLOAD_PATH := $(UPLOAD_PATH_$(OS))

# Make Rule Constants

compile := $(BUILD_PATH)/$(TARGET).o

peripheral_src := $(wildcard $(LIB_PERIPHERALS_SRC)*.c)
peripheral_obj := $(patsubst $(LIB_PERIPHERALS_SRC)%.c,$(OBJ_PATH)/%.o,$(peripheral_src))

utils_src := $(wildcard $(UTILS_PATH)/*.c)
utils_obj := $(patsubst $(UTILS_PATH)/%.c,$(OBJ_PATH)/%.o,$(utils_src))
utils_first := $(patsubst $(UTILS_PATH)/%.c,$(CLOSE_UTILS_PATH)/%.o,$(close_utils_src))

bsp_src := $(wildcard $(LIB_BSP)/*.c)
bsp_obj := $(patsubst $(LIB_BSP)/%.c, $(OBJ_PATH)/%.o, $(bsp_src))

ext_out := $(patsubst lib%, $(OBJ_PATH)/%.o, $(DEPS))
ext_src := $(join $(DEPS:%=$(EXT_DIR)/%/src/), $(DEPS:lib%=%.c))
ext_inc := $(patsubst %, -I'$(EXT_DIR)/%/src/include/', $(DEPS))

assemble-startup := $(OBJ_PATH)/startup_stm32g474xx.o
build-library := $(ARCHIVE_PATH)/libperipherals.a

link := $(BUILD_PATH)/$(TARGET).elf
binary := $(BUILD_PATH)/$(TARGET).bin

# Compiler flags
CFLAGS = -std=gnu99 -g -O2 -Wall -mthumb -mcpu=$(CPU) -march=$(ARCH) --specs=nosys.specs
LFLAGS = -Xlinker --output=$(BUILD_PATH)/$(TARGET).elf -Xlinker \
--script=$(TOOLS_PATH)/STM32G474RETX_FLASH.ld -n
#-fno-exceptions 
#-nostartfiles

#-nostdlib -nodefaultlibs
LIB_INCLUDES = -I'$(LIB_CMSIS_DEV_INC)' -I'$(LIB_CMSIS_INC)' -I'$(LIB_PERIPHERALS_INC)' -I'$(LIB_BSP)' 
LIB_INCLUDES_LOCAL = -I'$(SOURCE_PATH)' -I'$(UTILS_PATH)' -I'$(INCLUDE_PATH)' $(ext_inc)
LIB_LINKS = -L'$(ARCHIVE_PATH)' -L'$(BUILD_PATH)' -L'$(OBJ_PATH)'

# Preprocessor Symbols
PREPROC_DEVICE := -D STM32G474xx
PREPROC_DRIVER := -D USE_STDPERIPH_DRIVER
PREPROC_NUCLEO := -D USE_NUCLEO_64
PREPROC_LIBFIXED := -DCONFIG_FRAC_BITWIDTH=5 -DCONFIG_BITWIDTH=16
PREPROC_LIBMAT := -DBLOCK_SIZE=128 -DINPUT_SIZE=3
PREPROC_LOCAL := -DWORKLOAD=$(WORKLOAD)

PREPROC_SYMS := $(PREPROC_DEVICE) $(PREPROC_DRIVER) $(PREPROC_NUCLEO) $(PREPROC_LIBFIXED) \
				$(PREPROC_LIBMAT) $(PREPROC_LOCAL)

# Make Rules

all: $(utils_obj) $(ext_out) $(peripheral_obj) $(bsp_obj) $(assemble-startup) $(build-library) \
		$(compile) $(link) $(binary)

first: $(utils_first)

$(OBJ_PATH)/%.o: $(UTILS_PATH)/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(LIB_INCLUDES_LOCAL) $(PREPROC_SYMS) -c -o $@ $<
	#mv $(UTILS_PATH)/$*.o $(OBJ_PATH)

# upload for windows
upload-win: $(BUILD_PATH)/$(TARGET).bin
	cp $< $(UPLOAD_PATH_WIN)/$(<F)

# upload for ubuntu
upload-ubuntu: $(BUILD_PATH)/$(TARGET).bin
	cp $< $(UPLOAD_PATH_UBUNTU)/$(<F)

upload-arch: $(BUILD_PATH)/$(TARGET).bin
	cp $< $(UPLOAD_PATH_ARCH)/$(<F)

# Figure out the OS and upload
upload: $(BUILD_PATH)/$(TARGET).bin
	sudo cp $< $(UPLOAD_PATH)

# build library
$(build-library): $(peripheral_obj) $(bsp_obj)
	$(AR) rcs $@ $^

# link
$(link): $(BUILD_PATH)/$(TARGET).o $(assemble-startup) $(build-library) $(utils_obj) $(ext_out)
	$(CC) $(CFLAGS) $(LFLAGS) $(LIB_LINKS) -lc $^

# binary
$(binary): $(BUILD_PATH)/$(TARGET).elf
	$(CP) -O binary $< $@

# compile
$(compile): $(SOURCE_PATH)/$(TARGET).c $(INCLUDE_PATH)/$(TARGET).h 
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(LIB_INCLUDES_LOCAL) $(PREPROC_SYMS) -c $< -o $@

# assemble-startup
$(assemble-startup): $(patsubst $(OBJ_PATH)%.o, $(TOOLS_PATH)%.s ,$(assemble-startup))
	$(CC) $(CFLAGS) -c -o $@ $<


# peripheral_obj
$(OBJ_PATH)/%.o:: $(LIB_PERIPHERALS_SRC)%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(LIB_INCLUDES_LOCAL) $(PREPROC_SYMS) -c -o $@ $<

# bsp_obj
$(OBJ_PATH)/%.o:: $(LIB_BSP)%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(LIB_INCLUDES_LOCAL) $(PREPROC_SYMS) -c -o $@ $<

# ext_out
$(OBJ_PATH)/%.o:: $(EXT_DIR)/lib%/src/*.c
	$(CC) $(CFLAGS) $(ext_inc) $(LIB_INCLUDES) $(LIB_INCLUDES_LOCAL) $(PREPROC_SYMS) -c $< -o $@

objdump: $(link)
	$(DUMP) -drwC -S $^

clean:
	rm -f $(BUILD_PATH)/$(TARGET).* 

clean-all: 
	rm -f $(peripheral_obj) $(assemble-device) $(assemble-startup) $(assemble-nucleo) $(assemble-it-file) $(build-library) $(BUILD_PATH)/$(TARGET).*

clean-peripherals:
	rm -f $(peripheral_obj)

clean-obj:
	rm -f $(OBJ_PATH)/*.o

clean-ar:
	rm -f $(ARCHIVE_PATH)/*.a

clean-locals:
	rm -f $(assemble-device) $(assemble-it-file) $(assemble-nucleo) $(assemble-startup)

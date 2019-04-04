TARGET = compress

CC = msp430-elf-gcc -mmcu=msp430fr5994

all:
	CC $(TARGET).c -lm -o $(TARGET).o


  6 MSP430_SOURCE = /opt/ti/msp430-gcc
  7 CC = $(MSP430_SOURCE)/bin/msp430-elf-gcc
  8 AR = $(MSP430_SOURCE)/bin/msp430-elf-ar
  9 CP = $(MSP430_SOURCE)/bin/msp430-elf-objcopy
 10 DBG = mspdebug
 11
 12 CFLAGS = -std=c99 -Wall -minrt -fno-builtin -O2 -g
 13 INCLUDE_FLAGS = -I $(MSP430_SOURCE)/include/ -I src/ -I inc/
 14 MCU = msp430fr5994
 15 INCLUDE_BASE_LIBRARIES = /usr/include/
 16
 17 SOURCES = $(wildcard inc/*.c)
 18 OBJECTS = $(patsubst inc/%.c,bld/%.o,$(SOURCES))
 19 BUILD_LIBRARY = bld/library.a
 20 LINK = $(TARGET).elf
 21 BINARY = $(TARGET).bin
 22 OUTPUT=out.elf
 23
 24 STRIPARGS=-Os -ffunction-sections -Wl,--gc-sections -fno-asynchronous-unwind-tables -Wl,--strip-all
 25 DBGARGS=rf2500
 26
 27 #all: compile
 28 all: $(OBJECTS) $(BUILD_LIBRARY) compile $(LINK) $(BINARY)
 29
 30 test:
 31   $(CC) $(CFLAGS) -D __MSP430FR5994__ $(INCLUDE_FLAGS) -I /usr/include/ -mmcu=$(MCU) -c src/sx1276.c -o sx1276.o -lm
 32
 33 bld/%.o: inc/%.c
 34   $(CC) $(CFLAGS) -D __MSP430FR5994__ $(INCLUDE_FLAGS) -I $(INCLUDE_BASE_LIBRARIES) -mmcu=$(MCU) -c $< -o $@
 35
 36 $(BUILD_LIBRARY): $(OBJECTS)
 37   $(AR) rcs $@ $^
 38
 39 compile: src/$(INPUT).c
 40   $(CC) $(CFLAGS) -D __MSP430FR5994__ $(INCLUDE_FLAGS) -mmcu=$(MCU) -c src/$(INPUT).c -o $(TARGET).o -lm
 41
 42 $(LINK): $(TARGET).o $(BUILD_LIBRARY)
-- VISUAL --                                                                                                                                                                                     2,1           Top


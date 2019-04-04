TARGET = compress

MSP430_SOURCE = /opt/ti/mspgcc
#CC = $(MSP430_SOURCE)/bin/msp430-elf-gcc
CC = gcc
CFLAGS = -std=c99 -Wall -O2 -g
MCU = msp430fr5994

all:
	#$(CC) $(CFLAGS) -D __MSP430FR5994__ -I $(MSP430_SOURCE)/include/ -mmcu=$(MCU)	-lc $(TARGET).c -lm
	$(CC) $(CFLAGS) -lc $(TARGET).c -lm


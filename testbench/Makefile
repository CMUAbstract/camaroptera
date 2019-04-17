TARGET = compress

MSP430_SOURCE = /opt/ti/mspgcc
#CC = $(MSP430_SOURCE)/bin/msp430-elf-gcc
CC = gcc
CFLAGS = -std=c99 -Wall -O2 -g
MCU = msp430fr5994

INPUT_FILES = $(wildcard ./Dataset/Divided/*.txt)

all: main

main1:
	#$(CC) $(CFLAGS) -D __MSP430FR5994__ -I ./ -I $(MSP430_SOURCE)/include/ -mmcu=$(MCU)	-lc $(TARGET).c -lm
	#$(CC) $(CFLAGS) -I ./ -lc $(TARGET).c -lm -o $(TARGET).out

main:
	$(CC) $(CFLAGS) -I ./ -lc $(TARGET).c -lm -c

exec:
	for file in $(INPUT_FILES); do \
		./a.out $$file; \
	done

clean:
	rm *.out

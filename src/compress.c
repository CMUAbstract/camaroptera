#include "compress.h"

extern uint8_t frame[];

__nv uint8_t frame2[];	// Need to define this the same as frame[]

void rle(uint16_t nb){		// Need to add markers to avoid negative compression
		uint16_t i, j, count;

		j = 0;

		for( i = 0; i < nb; i++ ){
			count = 1;

			while( i  < nb-1 && frame[i] == frame[i+1]){
					count++;
					i++;
				}

			// Need to change the stored location
			frame2[j] = count;
			frame2[j+1] = frame[i];
			j += 2;

			}
	}

void lzw(uint16_t nb){
	
	
	}

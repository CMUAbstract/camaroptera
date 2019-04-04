//#include <msp430.h>
#include <stdint.h>
#include "compress.h"
#include <stdio.h>

unsigned char run_ct, A, B, RLE_flag;
size_t count;

uint16_t rle(){
		uint16_t i, j, k, run_count;
		uint8_t RLE_flag = 0;

		j = 0;
		run_count = 1;

		for( i = 0; i < nb; i++ ){
			
			printf("frame[%d] = %d | frame2[%d] = %d \n", i, frame[i], j, frame2[j]);

			if( frame[i] == frame[i+1] ){ // If there is a contiguous string of same bytes
				printf("Increasing counter\n");
				run_count ++;
				}
			else{
				if(run_count > 1){
					if(RLE_flag == 0){
						frame2[j] = RLE_flag;
						printf("frame2[%d] = %d\n", j, frame2[j]); 
						j++;
					}
					RLE_flag = 1;
					printf("Here: count=%d\n", run_count);
					frame2[j] = run_count;
					printf("Write run count - frame2[%d] = %d\n", j, frame2[j]); 
					frame2[j+1] = frame[i];
					printf("Write data - frame2[%d] = %d\n", j+1, frame2[j+1]); 
					j += 2;
					run_count = 1;
				}
				else if(run_count == 1){
					printf("There\n");
					if(frame[i] == 0){
						if(RLE_flag == 1){
							frame2[j] = run_count;
							printf("run count RLE1 - frame2[%d] = %d\n", j, frame2[j]); 
							j++;
						}
						else{
							frame2[j] = RLE_flag;
							printf("RLE flag 0 - frame2[%d] = %d\n", j, frame2[j]); 
							frame2[j+1] = run_count;
							printf("run count RLE0 - frame2[%d] = %d\n", j+1, frame2[j+1]); 
							RLE_flag = 1;
							j += 2;
						}
					}
					else{
						if( RLE_flag == 1 ){
							RLE_flag = 0;
							frame2[j] = RLE_flag;
							printf("RLE flag 1 -> 0 - frame2[%d] = %d\n", j, frame2[j]); 
							j++;
						}
					}
				frame2[j] = frame[i];
				printf("Final write - frame2[%d] = %d\n", j, frame2[j]); 
				j++;
				}
			}
			
			// Need to change the stored location
			if(j > nb || i > 100){
				printf("Entering Negative Compression\n");
				break;
				}

			}

		return j;
}

int main(){

	printf("Compressing. . .\n");	
	uint16_t temp = rle();
	printf("Size of compressed file: %d\n", temp);
	}

//#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "compress.h"

unsigned char run_ct, A, B, RLE_flag;
size_t count;

int main( int argc, char *argv[] ){

	char * input_filename = argv[1];

	float comp_ratio;

	file_read( input_filename );

	printf("========================================\n");
	printf("Compressing. . .\n");	
	uint16_t temp = rle(false);
	printf("Size of compressed file: %d\n", temp);
	comp_ratio = 100*(((float)nb - (float)temp)/(float)nb);
	printf("Reduction in size: %f %% \n", comp_ratio);
	printf("========================================\n");

}

void file_read( char * input_filename){

	FILE *fp = fopen( input_filename, "rb" );
	unsigned char * number_read = malloc(4);
	unsigned char number;
	int i, bytes, temp;

	uint16_t read_count = 0;

	while(1){
		bytes = 0;
		number = 0;

		//printf("Temp = %d\n", temp);

		while ((temp = fgetc(fp)) != 32){
			if (temp == -1){
				break;
			}
			number_read[bytes] = temp;
			//printf("%d(%d) = %d | %d\n", temp, bytes, temp-48, number_read[bytes]);
			bytes++;
		}

		for( i = 0; i < bytes; i++ ){
			number += (number_read[i] - 48)*(int)pow(10,bytes - i - 1);
			}

		frame[read_count] = number;
		//printf("Number decoded: %d | ", number);
		//printf("Read Count = %d\n", read_count);
		read_count ++;
		if (temp == -1){
			break;
		}

	}

	printf("Read %d bytes from file: %s\n", read_count, input_filename);
	fclose(fp);

	}

// ==================== RLE CODE BEGINS HERE ========================

uint16_t rle(bool quantization_on){
		uint16_t i, j, k, run_count;
		uint8_t RLE_flag = 0;

		j = 0;
		run_count = 1;

		for( i = 0; i < nb; i++ ){
			
			//printf("frame[%d] = %d | frame2[%d] = %d \n", i, frame[i], j, frame2[j]);
			if(quantization_on == true){
				frame[i] = frame[i]/5;
				frame[i] = frame[i]*5;

				frame[i+1] = frame[i+1]/5;
				frame[i+1] = frame[i+1]*5;
			}

			if( frame[i] == frame[i+1] ){ // If there is a contiguous string of same bytes
				//printf("Increasing counter\n");
				run_count ++;
				}
			else{
				if(run_count > 1){
					if(RLE_flag == 0){
						frame2[j] = RLE_flag;
						//printf("frame2[%d] = %d\n", j, frame2[j]); 
						j++;
					}
					RLE_flag = 1;
					//printf("Here: count=%d\n", run_count);
					frame2[j] = run_count;
					//printf("Write run count - frame2[%d] = %d\n", j, frame2[j]); 
					frame2[j+1] = frame[i];
					//printf("Write data - frame2[%d] = %d\n", j+1, frame2[j+1]); 
					j += 2;
					run_count = 1;
				}
				else if(run_count == 1){
					//printf("There\n");
					if(frame[i] == 0){
						if(RLE_flag == 1){
							frame2[j] = run_count;
							//printf("run count RLE1 - frame2[%d] = %d\n", j, frame2[j]); 
							j++;
						}
						else{
							frame2[j] = RLE_flag;
							//printf("RLE flag 0 - frame2[%d] = %d\n", j, frame2[j]); 
							frame2[j+1] = run_count;
							//printf("run count RLE0 - frame2[%d] = %d\n", j+1, frame2[j+1]); 
							RLE_flag = 1;
							j += 2;
						}
					}
					else{
						if( RLE_flag == 1 ){
							RLE_flag = 0;
							frame2[j] = RLE_flag;
							//printf("RLE flag 1 -> 0 - frame2[%d] = %d\n", j, frame2[j]); 
							j++;
						}
					}
				frame2[j] = frame[i];
				//printf("Final write - frame2[%d] = %d\n", j, frame2[j]); 
				j++;
				}
			}

		/*	// Need to change the stored location
			if(j > nb || i > 100){
				printf("Entering Negative Compression\n");
				break;
				}
*/
			}

		return j;
}
// ==================== RLE CODE ENDS HERE ========================

// ==================== LZW CODE BEGINS HERE ========================

// ==================== LZW CODE ENDS HERE ========================


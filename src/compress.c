#include "compress.h"

#define enable_debug

extern uint8_t frame[];

__nv uint8_t frame2[];	// Need to define this the same as frame[]

bool rle(uint16_t nb){
		uint16_t i, j, k, run_count;
		uint8_t RLE_flag, success_flag;
		char temp[100];
		j = 0;
		run_count = 1;
		RLE_flag = 0;
		success_flag = 1;

		for( i = 0; i < nb; i++ ){

#ifdef enable_debug
			sprintf(temp, "frame[%d] = %d | frame2[%d] = %d \n", i, frame[i], j, frame2[j]);
			uart_write(temp);
#endif

			if( frame[i] == frame[i+1] ){ // If there is a contiguous string of same bytes
#ifdef enable_debug
				uart_write("Increasing counter\n");
#endif
				run_count ++;
				}
			else{
				if(run_count > 1){
					if(RLE_flag == 0){
						frame2[j] = RLE_flag;
#ifdef enable_debug
						uart_write("frame2[%d] = %d\n", j, frame2[j]); 
#endif
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
			if(j > nb){
				printf("Entering Negative Compression\n");
				success_flag = 0;
				break;
				}

			}

		if(success_flag)
			return true;
		else
			return false;
}

bool lzw(uint16_t nb){
	
	
	}

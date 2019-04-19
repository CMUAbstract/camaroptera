#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <msp430.h>
#include "hog-svm.h"
#include "svm.h"
#include "input.h"

#include <libfixed/fixed.h>
//#include <libmspmath/msp-math.h>

#include <libio/console.h>

#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#define PI 3.14159265

uint8_t modulo( uint8_t , uint8_t );



int main(void){

	msp_watchdog_disable();

	msp_gpio_unlock();

	msp_clock_setup();
	//INIT_CONSOLE();


	P8OUT &= ~BIT0; 
	P8DIR |= BIT0;

	//PRINTF("Start\r\n");
	P8OUT |= BIT0;
	sobel(120, 160);
	sobel(4, 4);
	P8OUT &= ~BIT0; 

	//PRINTF("Processing\r\n");
	P8OUT |= BIT0;
	uint16_t count = histogram(120, 160, 8, 8, 2, 2);
	P8OUT &= ~BIT0; 
	
	//PRINTF("Processed\r\n");
	
	P8OUT |= BIT0;
	fixed result = infer();
	P8OUT &= ~BIT0; 
	
	//PRINTF("%i\r\n", result);
	
	//if( result < 0 )
		//PRINTF("PERSON DETECTED !\r\n");
	//else
		//PRINTF("No Person :(\r\n");

	return 0;
}

// ==================== FILE I/O BEGINS HERE ========================
/*
uint16_t file_read( char * input_filename){

	FILE *fp;

	if( ( fp = fopen( input_filename, "rb") ) == NULL ){
	       printf("Cannot open input-file %s.\n",input_filename);
	       exit(0);
	}

	unsigned char * number_read = malloc(4);
	unsigned char number;
	int i, bytes, temp;

	uint16_t read_count = 0;

	while(1){
		bytes = 0;
		number = 0;

		//printf("Temp = %d\n", temp);

		while ((temp = fgetc(fp)) != 32 && (temp > 47) && (temp < 58)){
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
	return read_count;

	}


uint16_t file_write( char * output_filename, uint16_t write_count){

	FILE *fp;

	if( ( fp = fopen( output_filename, "wb") ) == NULL ){
	       printf("Cannot open output-file %s.\n", output_filename);
	       exit(0);
	}

	uint16_t i;

	for( i = 0; i < write_count; i++ ){
		fprintf( fp, "%d ", frame2[i]);
		
		} 
	printf("Wrote %d bytes to file: %s\n", write_count, output_filename);
	fclose(fp);
	return 1;

	}*/
// ==================== FILE I/O ENDS HERE ========================

// ================================= SOBEL BEGINS HERE ====================================

uint16_t sqrt16(uint32_t x)
{
    uint16_t hi = 0xffff;
    uint16_t lo = 0;
    uint16_t mid = ((uint32_t)hi + (uint32_t)lo) >> 1;
    uint32_t s = 0;

    while (s != x && hi - lo > 1) {
        mid = ((uint32_t)hi + (uint32_t)lo) >> 1;
        s = (uint32_t)mid * (uint32_t)mid;
        if (s < x)
            lo = mid;
        else
            hi = mid;
    }

    return mid;
}

void sobel(uint8_t height, uint8_t width){
	uint8_t i, j;
	uint16_t pixel;
	float angle_temp;
	uint32_t square;
	uint16_t sqrt_temp;

	int16_t x_temp, y_temp;
	
	for( i = 0; i < height; i++){
		for( j = 0; j < width; j++){

			pixel = i*width + j;
			x_temp = y_temp = 0;

			if( j == 0 || j == width - 1 )
				x_temp = 0;
			else
				x_temp = frame[pixel+1] - frame[pixel-1];

			if( i == 0 || i == height - 1 )
				y_temp = 0;
			else
				y_temp = frame[pixel+width] - frame[pixel-width];
			
			square = (x_temp*x_temp) + (y_temp*y_temp);
			if(square == 0)
				sqrt_temp = 0;
			else{
				sqrt_temp = sqrt16(square);
			}
			
			g[pixel] = sqrt_temp;

			if(square == 0)
				angle_temp = 0.0;
			else if(x_temp == 0)
				angle_temp = 100; // Arbitrary value larger than 5.67 to bin it in 80 deg
			else
				angle_temp = (float)y_temp/(float)x_temp;

			theta[pixel] = F_LIT(angle_temp);
			}
		}
	}

// ================================= SOBEL ENDS HERE ====================================

// ================================= HISTOGRAM BEGINS HERE ====================================

uint8_t map_index( float x ){
		if( x >= 0 && x < 0.36)
        return 0;
    else if( x >= 0.36 && x < 0.84)
        return 1;
    else if( x >= 0.84 && x < 1.73)
        return 2;
    else if( x >= 1.73 && x < 5.67)
        return 3;
    else if( x < -5.67 || x >= 5.67)
        return 4;
    else if( x >= -5.67 && x < -1.73 )
        return 5;
    else if( x >= -1.73 && x < -0.84)
        return 6;
    else if( x >= -0.84 && x < -0.36)
        return 7;
    else if( x >= -0.36 && x < 0)
        return 8;
		return 255;
}

uint16_t histogram(uint8_t height, uint8_t width, uint8_t rows_per_cell, uint8_t cols_per_cell, uint8_t row_cell_per_block, uint8_t col_cell_per_block){
	uint16_t temp[9];
	uint8_t i, i1, j, j1, k, l, index;
	uint16_t pixel, count16;
	uint32_t sum;
	fixed hist16_temp;

	uint8_t cells_in_x_dir = (uint8_t)width/cols_per_cell;
	uint8_t cells_in_y_dir = (uint8_t)height/rows_per_cell;
	uint8_t strides_in_x_dir = cells_in_x_dir - col_cell_per_block + 1;
	uint8_t strides_in_y_dir = cells_in_y_dir - row_cell_per_block + 1;
	float a;

	for( i = 0; i < 9; i++ )
		temp[i] = 0;

	for( i = 0; i < cells_in_y_dir; i++ ){
		for( j = 0; j < cells_in_x_dir; j++ ){
			for( k = 0; k < rows_per_cell; k++ ){
				for( l = 0; l < cols_per_cell; l++ ){
					pixel = (i*cols_per_cell + k)*width + j*rows_per_cell + l;
					a	= F_TO_FLOAT(theta[pixel]);

					index = map_index(a);
					temp[index] += g[pixel];
				}
			}
			// End of 8x8 Loop
			for( k = 0; k < 9; k++ ){
				pixel =((i*cells_in_x_dir)+j)*9+k; 
				hist8x8[pixel] = temp[k];
				temp[k] = 0;
			}

		}
	} // End of initial histogram

	uint16_t count = 0;

	count16 = 0;

	for( i = 0; i < strides_in_y_dir; i++ ){
		for( j = 0; j < strides_in_x_dir; j++ ){
			pixel = (i*cells_in_x_dir + j)*9;
			sum = 0;
			for( i1 = 0; i1 < col_cell_per_block; i1 ++ ){
				for( j1 = 0; j1 < row_cell_per_block; j1 ++ ){
					for( k = 0; k < 9; k++ ){
						pixel =((i+i1)*cells_in_x_dir + (j+j1))*9 + k; 
						sum += (double)((double)(hist8x8[pixel]) *	(double)(hist8x8[pixel]));
						}
				}
			}
			count ++;
			sum = sqrt16(sum);

			for( i1 = 0; i1 < col_cell_per_block; i1++){
				for( j1 = 0; j1 < row_cell_per_block; j1++){
					for( k = 0; k < 9; k++ ){
						pixel = ((i+i1)*cells_in_x_dir + j + j1)*9 +	k;
						hist16_temp = F_LIT(hist8x8[pixel]);
						hist16_temp =  F_DIV(hist16_temp, F_LIT(sum));
						hist16x16[count16] = hist16_temp;
						count16 ++;
					}
				}
			}

		}
	}
	return count16;
}
// ================================= HISTOGRAM ENDS HERE ====================================

// ================================= INFERENCE BEGINS HERE ====================================

fixed infer(){
	uint16_t i;
	fixed result = 0;
	for( i = 0; i < 9576; i++ ){
		result +=  F_MUL( hist16x16[i], svm_w_0[i] );
		}
	result += F_ADD(result, svm_b_0);

	return result;
}

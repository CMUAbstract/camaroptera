#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <msp430.h>
#include "hog-svm.h"
#include "svm.h"
#include "input.h"

#include <libfixed/fixed.h>
#include <libmspmath/msp-math.h>

#include <libio/console.h>

#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#define PI 3.14159265

uint8_t modulo( uint8_t , uint8_t );


int main( int argc, char *argv[] ){

	msp_watchdog_disable();

	msp_gpio_unlock();

	msp_clock_setup();
	INIT_CONSOLE();

	sobel(120, 160);

	histogram(120, 160);

	double result = infer();
	if( result < 0 )
		PRINTF("PERSON DETECTED !\r\n");
	else
		PRINTF("No Person :(\r\n");

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

uint8_t modulo( uint8_t first, uint8_t second ){
	if ( first > second )
		return (first - second);
	else
		return (second - first);
	}

void sobel(uint8_t height, uint8_t width){
	uint8_t i, j, a, b, c, d;
	uint16_t pixel;
	float angle_temp;
	uint32_t square;
	uint16_t sqrt_temp;

	int16_t x_temp, y_temp;

	for( i = 0; i < height; i++){
		for( j = 0; j < width; j++){

			pixel = i*width + j;

			if( i == 0 ){
				//For left and top border pixels
				if( j == 0 ){
					
					a = frame[pixel+1];
					b = 0;
				}
				else if( j == width-1 ){
					a = 0;
					b = frame[pixel-1];
				}
				else{
					a = frame[pixel+1];
					b = frame[pixel-1];
				}
					c = frame[pixel+width];
					d = 0;
				}
			else if( i == height-1){
				//For right and bottom border pixels
				if( j == 0 ){
					
					a = frame[pixel+1];
					b = 0;
				}
				else if( j == width-1 ){
					a = 0;
					b = frame[pixel-1];
				}
				else{
					a = frame[pixel+1];
					b = frame[pixel-1];
				}
					c = 0;
					d = frame[pixel-width];
				}
			else{
				a = frame[pixel+1];
				b = frame[pixel-1];
				c = frame[pixel+width];
				d = frame[pixel-width];
				}

			x_temp = a - b;
			y_temp = c - d;

			square = (x_temp*x_temp) + (y_temp*y_temp);
			sqrt_temp = sqrt16(square);
			g[pixel] = sqrt_temp;


			angle_temp = (double)(atan2((float)y_temp,(float)x_temp) * 180.0/ PI);
			angle_temp = y_temp/x_temp;

			theta[pixel] = F_LIT(angle_temp);
			}
		}
	}

// ================================= SOBEL ENDS HERE ====================================

// ================================= HISTOGRAM BEGINS HERE ====================================

void histogram(uint8_t height, uint8_t width){
	uint16_t temp[9];
	uint8_t i, i1, j, j1, k, l, lower, upper;
	uint16_t pixel, count16;
	uint8_t x, y;
	double sum, hist16_temp;

	uint16_t a;
	fixed b, z;

	for( i = 0; i < 9; i++ )
		temp[i] = 0;

	for( i = 0; i < 15; i++ ){
		for( j = 0; j < 20; j++ ){
			for( k = 0; k < 8; k++ ){
				for( l = 0; l < 8; l++ ){
					pixel = (i*8 + k)*width + j*8 + l;

					a	= (uint16_t)F_TO_FLOAT(theta[pixel]);

					x = a % 20;
					y = a/20;

					lower = y%9;
					upper = (y+1)%9;

					b = F_DIV( F_LIT(x), F_LIT(20) );
					z = F_MUL( g[pixel], b );
					temp[upper] += F_TO_FLOAT(z);
					temp[lower] += F_TO_FLOAT( F_SUB( g[pixel], z ) );
				}
			}
			// End of 8x8 Loop
			for( k = 0; k < 9; k++ ){
				hist8x8[((i*20)+j)*9+k] = temp[k];
				temp[k] = 0;
			}
		}
	} // End of initial histogram

	uint16_t count = 0;

	count16 = 0;

	for( i = 0; i < 14; i++ ){
		for( j = 0; j < 19; j++ ){
			pixel = (i*20 + j)*9;
			sum = 0;
			for( k = 0; k < 9; k++ ){
				sum += ((hist8x8[(i*20 + j)*9 + k]) * (hist8x8[(i*20 + j)*9 + k]));
				sum += ((hist8x8[((i+1)*20 + j)*9 + k]) * (hist8x8[((i+1)*20 + j)*9 + k]));
				sum += ((hist8x8[(i*20 + (j+1))*9 + k]) * (hist8x8[(i*20 + (j+1))*9 + k]));
				sum += ((hist8x8[((i+1)*20 + (j+1))*9 + k]) * (hist8x8[((i+1)*20 + (j+1))*9 + k]));
				}
			count ++;
			sum = sqrt16(sum);

			for( i1 = 0; i1 < 2; i1++){
				for( j1 = 0; j1 < 2; j1++){
					for( k = 0; k < 9; k++ ){
						hist16_temp = (hist8x8[((i+i1)*20 + j + j1)*9 +	k])/sum;
						hist16x16[count16] = F_LIT(hist16_temp);
						count16 ++;
					}
				}
			}

		}
	}

}
// ================================= HISTOGRAM ENDS HERE ====================================

// ================================= INFERENCE BEGINS HERE ====================================

double infer(){
	uint16_t i;
	double result = 0;

	for( i = 0; i < 9576; i++ ){
		result += F_TO_FLOAT( F_MUL( hist16x16[i], svm_w_0[i] ) );
		}
	result += F_TO_FLOAT(svm_b_0);

	return result;
}

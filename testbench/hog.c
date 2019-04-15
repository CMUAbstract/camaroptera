#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "hog.h"

#define PI 3.14159265

uint8_t gx[19200];
uint8_t gy[19200];
double g[19200];
double theta[19200];

int main( int argc, char *argv[] ){

	char * input_filename = argv[1];
	char * output_filename = argv[2];

	uint16_t read_count, write_count;

	printf("========================================\n");
	read_count = file_read( input_filename );

	printf("Running Sobel Filter. . .\n");	
	sobel(120, 160);
	histogram(120, 160);
	file_write(output_filename, nb);
	printf("========================================\n");
}

// ==================== FILE I/O BEGINS HERE ========================

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

	}
// ==================== FILE I/O ENDS HERE ========================

// ================================= SOBEL BEGINS HERE ====================================

uint8_t modulo( uint8_t first, uint8_t second ){
	if ( first > second )
		return (first - second);
	else
		return (second - first);
	}

void sobel(uint8_t height, uint8_t width){
	uint8_t i, j;
	uint16_t pixel;
	float angle_temp;

	for( i = 0; i < height; i++){
		for( j = 0; j < width; j++){
			pixel = i*width + j;
			if( i == 0  || j == 0 ){
				//For left and top border pixels
				//printf("Top/Left\n");
				gx[pixel] = frame[pixel+1];
				gy[pixel] = frame[pixel+width];
				}
			else if( i == height-1  || j == width-1 ){
				//For right and bottom border pixels
				//printf("Bottom/Right\n");
				gx[pixel] = frame[pixel-1];
				gy[pixel] = frame[pixel-width];
				}
			else{
				gx[pixel] = modulo(frame[pixel-1], frame[pixel+1]);
				gy[pixel] = modulo(frame[pixel-width], frame[pixel+width]);
				//printf("g[%d][%d] | f1x = %d | f2x = %d | gx = %d | f1y = %d | f2y = %d	| gy = %d\n", i, j, frame[pixel-1], frame[pixel+1],	gx[pixel], frame[pixel-width], frame[pixel+width], gy[pixel]);
				}
				g[pixel] = (double)sqrt( pow(gx[pixel], 2) + pow(gy[pixel], 2));
				angle_temp = (double)(atan2((float)gy[pixel],(float)gx[pixel]) * 180.0/ PI);
				if(angle_temp < 0.0)
					theta[pixel] = angle_temp + 180;
				else
					theta[pixel] = angle_temp;
				frame2[pixel] = g[pixel];
				// printf("g = %f | theta = %f\n", g[pixel], theta[pixel]);
				// printf("g[%d][%d] @ pixel = %d | gx = %d | gy = %d\n", i, j, pixel, gx[pixel], gy[pixel]);
			}
		}
	}

// ================================= SOBEL ENDS HERE ====================================

// ================================= HISTOGRAM BEGINS HERE ====================================

void histogram(uint8_t height, uint8_t width){
	uint16_t temp[9];
	uint8_t i, i1, j, j1, k, l, lower, upper;
	uint16_t pixel, count16;
	float x, y, z;
	double sum, max;

	for( i = 0; i < 9; i++ )
		temp[i] = 0;

	for( i = 0; i < 15; i++ ){
		for( j = 0; j < 20; j++ ){
			for( k = 0; k < 8; k++ ){
				for( l = 0; l < 8; l++ ){
					pixel = (i*8 + k)*width + j*8 + l;
//					printf("i = %d | j = %d | k = %d | l = %d | pixel = %d | mag = %d |	theta = %d\n", i, j, k, l, pixel, g[pixel], theta[pixel]);
					x = floor(theta[pixel])%20;
					y = theta[pixel]/20;
					lower = (uint8_t)floor(y)%9;
					upper = ((uint8_t)floor(y)+1)%9;
					z = (g[pixel]*x)/20;
					temp[upper] += z;
					temp[lower] += g[pixel] - z;
	//				printf("x = %d | y = %d | z = %d | lower = %d | upper = %d\n", x, y, z, lower, upper);
				}
			}
			// End of 8x8 Loop
			for( k = 0; k < 9; k++ ){
//				printf("%d(%d)\n", temp[k]);
				hist8x8[((i*20)+j)*9+k] = temp[k];
				temp[k] = 0;
			}
		}
	} // End of initial histogram

	uint16_t count = 0;
	max = 0;

	count16 = 0;

	for( i = 0; i < 14; i++ ){
		for( j = 0; j < 19; j++ ){
			pixel = (i*20 + j)*9;
			sum = 0;
			for( k = 0; k < 9; k++ ){
				sum += pow(hist8x8[(i*20 + j)*9 + k],2);
				sum += pow(hist8x8[((i+1)*20 + j)*9 + k],2);
				sum += pow(hist8x8[(i*20 + (j+1))*9 + k],2);
				sum += pow(hist8x8[((i+1)*20 + (j+1))*9 + k],2);

//				printf("i = %d | j = %d | pixel = %d | k = %d\n", i, j, pixel, k);
	//			printf("Hist1 = %d | Hist2 = %d | Hist3 = %d | Hist4 = %d | sum =	%d\n", hist[(i*20 + j)*9 + k],hist[((i+1)*20 + j)*9 + k], hist[(i*20 + j + 1)*9 + k], hist[((i+1)*20 + j + 1)*9 + k], sum);
				}
			count ++;
			sum = sqrt(sum);
			if(sum > max)
				max = sum;
//			printf("Start: ");

			for( i1 = 0; i1 < 2; i1++){
				for( j1 = 0; j1 < 2; j1++){
					for( k = 0; k < 9; k++ ){
						hist16x16[count16] = (float)(hist8x8[((i+i1)*20 + j + j1)*9 +	k])/sum;
						printf("%.5f (%d) | ", hist16x16[count16], (i1*2+j1)*9+k);
						count16 ++;
					}
				}
			}
			printf("\n\n");

		}
	}

	printf("total %d times. Max = %f\n", count16, max);
}
// ================================= HISTOGRAM ENDS HERE ====================================


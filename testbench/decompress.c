#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "codec.h"

uint8_t QUANTIZATION_FACTOR = 5;

int main( int argc, char *argv[] ){

	char * input_filename = argv[1];
	char * output_filename = argv[2];

	if(argc > 4)
		QUANTIZATION_FACTOR = atoi(argv[4]);

	printf("Quantization Factor: %d\n", QUANTIZATION_FACTOR);

	uint16_t read_count, write_count;

	read_count = file_read( input_filename );

	printf("========================================\n");
	printf("Decompressing. . .\n");	
	if(argc > 3){
		switch(atoi(argv[3])){
			case 0 :
							printf("Running RLE Decompression\n");
							write_count = decomp_rle(true, read_count);
							break;
			case 1 :
							printf("Running LZW Decompression\n");
							write_count = decomp_lzw(true, read_count);
							break;
			case 2 :
							printf("Running Huffman Decompression\n");
							write_count = decomp_huffman(true, read_count);
							break;
			default:
							printf("Select Decompression Algorithm: [0] RLE [1] LZW [2]Huffman\n");
							exit(0);
		}
	}
	printf("Size of compressed file: %d | Size of decompressed file: %d\n", read_count - 1, write_count);
	//comp_ratio = 100*(((float)nb - (float)temp)/(float)nb);
	//printf("Reduction in size: %f %% \n", comp_ratio);
	file_write(output_filename, write_count);
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
	return 1;

	}
// ==================== FILE I/O ENDS HERE ========================

// ==================== RLE DECOMPRESSION CODE BEGINS HERE ========================

void toggleBit(unsigned char * a){
	if(a[0] == 0)
		a[0] = 1;
	else
		a[0] = 0;

}

uint16_t decomp_rle(bool quantization_on, uint16_t input_size){

	uint8_t run_count, RLE_flag, read_byte;
	uint16_t in_count, out_count;
	clock_t start, end;
	
	// RLE Decompression.
	
	RLE_flag = 0;			// RLE is turned off by default.
	in_count = 0;
	out_count = 0;
	start = clock();
	
	for( in_count = 0; in_count < input_size;){
		run_count = frame[in_count];
		in_count ++;


		if(run_count == 0){												// If read byte is for RLE mode switch.
			if(RLE_flag == 0){
				run_count = frame[in_count];
				in_count ++;
			}
			else
				run_count = 1;
			toggleBit(&RLE_flag);
			read_byte = frame[in_count];
			in_count ++;
			while(run_count --){											// Write A for run_ct times to output-file.
				frame2[out_count] = read_byte;
				out_count ++;
			}
		}
		else{
			if(RLE_flag == 0){
				frame2[out_count] = read_byte;
				out_count ++;
			}
			else{
				read_byte = frame[in_count];
				in_count ++;
				while(run_count --){
					frame2[out_count] = read_byte;
					out_count ++;
				}
			}
		}
	}

	end = clock();

	printf("Decompression complete.\n");
	printf("Time taken to compress file: %.3lf seconds", (double) ( (end-start) / CLOCKS_PER_SEC ));
	
	return out_count;
}

// ==================== RLE DECOMPRESSION CODE ENDS HERE ========================

// ==================== LZW DECOMPRESSION CODE BEGINS HERE ========================
uint16_t addToDictionary ( uint8_t * buffer, uint16_t buffer_length, uint8_t ** entry_idx, uint8_t *** entry_row, uint16_t ** pattern_length, uint16_t current_row, uint16_t last_entry){
	uint16_t i, index;

	// Expand dictionary parameters by one entry.
	entry_idx[0] = (uint8_t *)realloc(entry_idx[0], (last_entry + buffer_length +	1)*sizeof(uint8_t));
	entry_row[0] = (uint8_t **)realloc(entry_row[0], (current_row +	1)*sizeof(uint8_t *));
	pattern_length[0] = (uint16_t *)realloc(pattern_length[0], (current_row +	1)*sizeof(uint16_t));

	// Reassign link between row pointer and entry pointer, since reallocation could change memory locations.
	index = 0;
	pattern_length[0][current_row] = buffer_length;
	for( i=0 ; i<= current_row; i++){
		entry_row[0][i] = &entry_idx[0][index];
		index += pattern_length[0][i];
	}

	// Add entry to dictionary.
	for( i = 1; i <= buffer_length; i++){
		entry_idx[0][ ++ last_entry] = buffer [ i-1 ];
	}

	// Return position of last element entered.
	return last_entry;
}

uint8_t * concat( uint8_t * next, uint8_t * prev, uint16_t prev_length){
	uint8_t * temp;
	uint16_t i;

	if(prev == NULL)
		return next;
	else{
		temp = (uint8_t *)malloc( ( prev_length + 1 ) * sizeof(uint8_t) );
		for(i=0; i<prev_length; i++)
			temp[i] = prev[i];
		temp[prev_length] = *next;
		return temp;
	}
}

uint16_t decomp_lzw(bool quantization_on, uint16_t input_size){
	uint16_t *entry_code;
	uint8_t *entry_idx, **entry_row, *new_char;
	uint16_t *pattern_length;
	uint16_t code_read, prev_code_read;
	uint16_t last_entry, current_row, initial_dict_length;
	uint16_t in_count, out_count;
	clock_t start, end;

	// Allocate memory for initial dictionary.
	entry_idx = (uint8_t *)malloc(256*sizeof(uint8_t));
	entry_row = (uint8_t **)malloc(256*sizeof(uint8_t *));
	entry_code = (uint16_t *)malloc(256*sizeof(uint16_t));
	pattern_length = (uint16_t *)malloc(256*sizeof(uint16_t));

	in_count = 0;
	out_count = 0;

	// Build Initial Dictionary.
	uint16_t i;
	printf("Building Initial Dictionary\n");

	for( i = 0; i <= 255; i++ ){
		entry_idx[i] = i;
		entry_row[i] = &entry_idx[i];
		entry_code[i] = i;
		pattern_length[i] = 1;
	}

	last_entry = 255;
	current_row = 256;

	initial_dict_length = current_row;
	printf("Initial Dictionary Built of length %d\n", initial_dict_length);

	start = clock();

	// Read C and output pattern for it.
	code_read = frame[in_count] | ((frame[in_count+1] << 8) & 0xFF00); // LSB First
	in_count += 2;
	for( i = 0; i < pattern_length[code_read]; i++){
		frame2[out_count] = entry_row[code_read][i];
		out_count++;
	}

	while(in_count < input_size - 1){

		// Let P = C.
		prev_code_read = code_read;

		// Read C.
		code_read = frame[in_count] | ((frame[in_count+1] << 8) & 0xFF00); // LSB First
		in_count += 2;

		if( code_read < current_row ){
			// C is present in dictionary.

			// Output pattern for C.
			for( i = 0; i < pattern_length[code_read]; i++){
				//printf("First - Pattern Length = %d\n", pattern_length[code_read]);
				frame2[out_count] = entry_row[code_read][i];
				out_count++;
			}

			// Generate X+Y. X = pattern for P. Y = 1st char of pattern for C.
			new_char = concat( entry_row[code_read], entry_row[prev_code_read], pattern_length[prev_code_read] );

			// Add X+Y to dictionary.
			last_entry = addToDictionary ( new_char, ( pattern_length [prev_code_read] + 1 ), &entry_idx, &entry_row, &pattern_length, current_row, last_entry);
			current_row ++;
			entry_code = (uint16_t *)realloc(entry_code, current_row*sizeof(uint16_t));
			entry_code[current_row-1] = current_row-1;
		}
		else{
			// C is not found in dictionary.
			// Generate X+Z. X = pattern for P. Z = 1st char of pattern for P.
			new_char = concat( entry_row[prev_code_read], entry_row[prev_code_read], pattern_length[prev_code_read] );

			// Output X+Z.
			for( i = 0; i < pattern_length[prev_code_read] + 1; i++){
				//printf("Second - Pattern Length = %d\n", pattern_length[code_read]);
				frame2[out_count] = new_char[i];
				out_count++;
			}

			// Add X+Z to dictionary.
			last_entry = addToDictionary ( new_char, ( pattern_length[prev_code_read] + 1 ), &entry_idx, &entry_row, &pattern_length, current_row, last_entry);
			current_row ++;
			entry_code = (uint16_t *)realloc(entry_code, current_row*sizeof(uint16_t));
			entry_code[current_row-1] = current_row-1;
		}

	}

	end = clock();
	printf("Final Dictionary Length: %d entries.\n", current_row);

	printf("Time taken to decompress file: %.3f seconds", (double) ( (end-start) / CLOCKS_PER_SEC ));

	free( entry_code );
	free( entry_idx );
	free( entry_row );
	free( pattern_length );
	free( new_char );

	return out_count;
}
// ==================== LZW DECOMPRESSION CODE ENDS HERE ========================

// ==================== HUFFMAN DECOMPRESSION CODE BEGINS HERE ========================
uint8_t getCode( uint16_t temp_code, uint8_t counter, uint8_t *length, uint16_t *array, uint16_t array_size, uint8_t *element, uint8_t *write_char){

	uint16_t i;

	for( i = 0 ; i < array_size ; i++ ){
		if( length[i] == counter ){
			if ( array[i] == temp_code){
				write_char[0] = element[i];
				return 1;
			}
		}
	}

	return 0;
}

uint16_t decomp_huffman(bool quantization_on, uint16_t input_size){

	uint16_t i;
	uint16_t read_count, array_size, temp_code, code_buffer;
	uint16_t in_count, out_count;
	uint16_t *code_array;

	uint8_t *element, *length;
	uint8_t write_char, code_read;

	uint8_t counter, code_counter, prev_counter;

	clock_t start, end;

	in_count = 0;
	out_count = 0;


	printf("Starting Decompression.\n");

	// -------------------------------------------------------------------------------------------------------------------------

	array_size = frame[in_count] | ((frame[in_count+1] << 8) & 0xFF00); // LSB First
	in_count += 2;

	element = (uint8_t *)malloc(array_size);
	length = (uint8_t *)malloc(array_size);
	code_array = (uint16_t *)malloc(array_size*sizeof(uint16_t));


	for ( i = 0 ; i < array_size ; i++ ){
		element[i] = frame[in_count];
		in_count++;
		length[i] = frame[in_count];
		in_count++;
		code_array[i] = frame[in_count] | ((frame[in_count+1] << 8) & 0xFF00); // LSB First
		in_count += 2;
	}

	read_count = frame[in_count] | ((frame[in_count+1] << 8) & 0xFF00); // LSB First
	in_count += 2;

	counter = 0;
	prev_counter = 0;
	code_buffer = 0;
	start = clock();

	for( i = 0; i < input_size - 1; i++ ){ // Read the file 4 bytes at a time.
		code_read = frame[in_count];
		in_count++;

		code_counter = 8 + counter;

		code_buffer |= ( code_read * (uint16_t)pow(2, counter));

		prev_counter = counter;

		while(code_counter > 0){ // Decode till all 4 bytes are decoded.

			counter = 1;
			while(1){

				temp_code = code_buffer & ( (uint16_t)pow(2, counter) - 1);

				if ( getCode( temp_code, counter, length, code_array, array_size, element, &write_char ) == 1){
					frame2[out_count] = write_char;
					out_count++;
					read_count--;
					code_read /= (uint16_t)pow(2, counter-prev_counter);
					code_buffer = code_read;

					#ifdef WRITE_PRINT
						printf ("Code Written: %c | Read Count: %ld | Remaining in buffer: %d | Counter = %d | Code counter = %d \n", write_char, read_count, code_buffer, counter, code_counter);
					#endif

					if( code_counter - counter == 0){
						code_counter-= counter;
						counter = 0;
					}
					prev_counter = 0;
					break;
				}
				else if( counter == code_counter)
					break;

				counter++;
			}

			if( read_count == 0)
				break;
			code_counter -= counter;
		}

		if( read_count == 0)
			break;
	}

	end = clock();
	printf("Decompression Complete.\nTime taken to decompress file: %.10lf seconds\n", (double) ( (end-start) / CLOCKS_PER_SEC ));	
	printf("Size of code table = %d\n", array_size);

	return out_count;
	}

// ==================== HUFFMAN DECOMPRESSION CODE ENDS HERE ========================


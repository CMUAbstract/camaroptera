//#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "codec.h"

//#define CODE_TABLE_PRINT
//#define OR_PRINT
//#define WRITE_PRINT

uint8_t QUANTIZATION_FACTOR = 5;


int main( int argc, char *argv[] ){

	char * input_filename = argv[1];
	char * output_filename = argv[2];

	if(argc > 4)
		QUANTIZATION_FACTOR = atoi(argv[4]);

	printf("Quantization Factor: %d\n", QUANTIZATION_FACTOR);

	float comp_ratio;
	uint16_t read_count, write_count;

	read_count = file_read( input_filename );

	printf("========================================\n");
	printf("Compressing. . .\n");	
	if(argc > 3){
		switch(atoi(argv[3])){
			case 0 :
							printf("Running RLE\n");
							write_count = rle(true);
							break;
			case 1 :
							printf("Running LZW\n");
							write_count = lzw(true);
							break;
			case 2 :
							printf("Running Huffman\n");
							write_count = huffman(true);
							break;
			default:
							printf("Select Compression Algorithm: [0] RLE [1] LZW [2]Huffman\n");
							exit(0);
		}
	}
	printf("Size of Original File: %d | Size of compressed file: %d\n", read_count-1, write_count);
	comp_ratio = 100*(((float)nb - (float)write_count)/(float)nb);
	printf("Reduction in size: %f %% \n", comp_ratio);
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
	return 1;

	}
// ==================== FILE I/O ENDS HERE ========================

// ==================== RLE CODE BEGINS HERE ========================

uint16_t rle(bool quantization_on){
		uint16_t i, j, run_count;
		uint8_t RLE_flag = 0;

		j = 0;
		run_count = 1;

		for( i = 0; i < nb; i++ ){

			if(quantization_on == true){
				frame[i] = frame[i]/QUANTIZATION_FACTOR;
				frame[i] = frame[i]*QUANTIZATION_FACTOR;

				frame[i+1] = frame[i+1]/QUANTIZATION_FACTOR;
				frame[i+1] = frame[i+1]*QUANTIZATION_FACTOR;
			}

			if( frame[i] == frame[i+1] ){ // If there is a contiguous string of same bytes
				run_count ++;
				}
			else{
				if(run_count > 1){
					if(RLE_flag == 0){
						frame2[j] = RLE_flag;
						j++;
					}
					RLE_flag = 1;
					frame2[j] = run_count;
					frame2[j+1] = frame[i];
					j += 2;
					run_count = 1;
				}
				else if(run_count == 1){
					if(frame[i] == 0){
						if(RLE_flag == 1){
							frame2[j] = run_count;
							j++;
						}
						else{
							frame2[j] = RLE_flag;
							frame2[j+1] = run_count;
							RLE_flag = 1;
							j += 2;
						}
					}
					else{
						if( RLE_flag == 1 ){
							RLE_flag = 0;
							frame2[j] = RLE_flag;
							j++;
						}
					}
				frame2[j] = frame[i];
				j++;
				}
			}

			}

		return j;
}
// ==================== RLE CODE ENDS HERE ========================

// ==================== LZW CODE BEGINS HERE ========================

int addToDictionary ( unsigned char * buffer, unsigned int buffer_length, unsigned char ** entry_idx, unsigned char *** entry_row, unsigned int ** pattern_length, unsigned int current_row, unsigned int last_entry){

	int i, index;

	// Expand dictionary parameters by one entry.
	entry_idx[0] = (unsigned char *)realloc(entry_idx[0], (last_entry + buffer_length + 1)*sizeof(unsigned char));
	entry_row[0] = (unsigned char **)realloc(entry_row[0], (current_row + 1)*sizeof(unsigned char *));
	pattern_length[0] = (unsigned int *)realloc(pattern_length[0], (current_row + 1)*sizeof(unsigned int));

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

int inDictionary ( unsigned char * buffer, unsigned int buffer_length, unsigned char ** entry_row, unsigned int * pattern_length, unsigned int current_row){

	int i, j, flag;

	if(current_row == 0)
		return -1;

	for( i = 0; i < current_row; i++){
		flag = 0;
		if( pattern_length[i] == buffer_length ){
			for( j = 0 ; j < buffer_length ; j++ ){
				if ( buffer[j] != entry_row[i][j]){		// Break if entry doesn't match.
					flag = 1;
					break;
				}
			}
			if( flag == 0){								// Match found in dictionary. Return entry row number.
				return i;
			}
		}

	}

	return -1;

}

unsigned char * concat( unsigned char * next, unsigned char * prev, int prev_length){

	unsigned char * temp;
	int i;

	if(prev == NULL)
		return next;
	else{
		temp = (unsigned char *)malloc( ( prev_length + 1 ) * sizeof(unsigned char) );
		for(i=0; i<prev_length; i++)
			temp[i] = prev[i];
		temp[prev_length] = *next;
		return temp;
	}
}

uint16_t lzw(bool quantization_on){
	// Variable Declaration.
	unsigned short int *entry_code, returned_code;
	unsigned char *entry_idx, **entry_row;
	unsigned int *pattern_length;
	unsigned char *char_read, *prev_char_read, *new_char;
	uint16_t in_count, out_count, last_entry, current_row, initial_dict_length, prev_length;
	clock_t start, end;

	// Allocate memory for initial dictionary.
	entry_idx = (unsigned char *)malloc(256*sizeof(unsigned char));
	entry_row = (unsigned char **)malloc(256*sizeof(unsigned char *));
	entry_code = (unsigned short int *)malloc(256*sizeof(unsigned short int));
	pattern_length = (unsigned int *)malloc(256*sizeof(unsigned int));
	char_read = (unsigned char *)malloc(sizeof(unsigned char));


	// Build Initial Dictionary.
	int i;
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

	prev_char_read = NULL;
	prev_length = 0;
	in_count = out_count = 0;

	start = clock();

	while(1){

		// Read char.
		*char_read = frame[in_count];
		in_count ++;
		if( in_count > nb )			// Break if EOF.
			break;

		if(quantization_on == true){
				*(char_read)= *(char_read)/QUANTIZATION_FACTOR;
				*(char_read) = *(char_read)*QUANTIZATION_FACTOR;
		}

		// Generate P+C
		new_char = concat( char_read, prev_char_read, prev_length );
		
		if( inDictionary ( new_char, prev_length + 1, entry_row, pattern_length, current_row) == -1 ){
			// P+C not in dictionary.

			// Output code for P.
			returned_code = inDictionary ( prev_char_read, prev_length, entry_row, pattern_length, current_row);
			//fwrite( &returned_code, sizeof(unsigned short int), 1, op_file );

			// Writing to output array -- LSB first
			frame2[out_count] = returned_code & 0xFF;
			out_count++;
			frame2[out_count] = (returned_code >> 8) & 0xFF;
			out_count++;

			// Add P+C to dictionary.
			last_entry = addToDictionary ( new_char, prev_length+1, &entry_idx, &entry_row, &pattern_length, current_row, last_entry);
			current_row ++;
			entry_code = (unsigned short int *)realloc(entry_code, current_row*sizeof(unsigned short int));
			entry_code[current_row-1] = current_row-1;

			// Let P = C.
			prev_char_read = (unsigned char *)realloc(prev_char_read, sizeof(unsigned char));
			*prev_char_read = *char_read;
			prev_length = 1;
		}
		else{
			// P+C found in dictionary.

			// Let P = P+C.
			prev_length ++;
			prev_char_read = (unsigned char *)realloc(prev_char_read, (prev_length) * sizeof(unsigned char));
			for(i=0; i<prev_length; i++)
				prev_char_read[i] = new_char[i];
		}

		if(current_row == 65536){
			printf("Maximum entries exceeded.\n");
			exit(0);
		}


	}

	uint16_t max = 0;

	// If no more data, output code for P and end.
	returned_code = inDictionary ( prev_char_read, prev_length, entry_row, pattern_length, current_row);
	printf("Final Dictionary Length: %d entries.\n", current_row);
	for( i = 0; i < current_row; i++ ){
//		printf("CODE ID %d --- >   Length = %d\n", i,  pattern_length[i]);
	if( pattern_length[i] > max)
			max = pattern_length[i];
		}
		printf("Longest Pattern Length= %d (%c)\n", max, 35);
	//fwrite( &returned_code, sizeof(unsigned short int), 1, op_file );
		// Writing to output array -- LSB first
	frame2[out_count] = returned_code & 0xFF;
	out_count++;
	frame2[out_count] = (returned_code >> 8) & 0xFF;
	out_count++;

	free( entry_code );
	free( entry_idx );
	free( entry_row );
	free( pattern_length );
	free( char_read );
	free( prev_char_read );
	free( new_char );
	end = clock();

	printf("Time taken to compress file: %.3lf seconds\n", (double) ( (end-start) / CLOCKS_PER_SEC ));
	return out_count;
}
// ==================== LZW CODE ENDS HERE ========================

// ==================== HUFFMAN CODE ENDS HERE ========================
typedef struct node{
	struct node *parent_node;
	uint16_t freq;
	uint16_t final_code_mask;
	uint8_t element;
	uint8_t code_bit;
	uint8_t code_length;
	//Disposable
	uint8_t *final_code_array;

}node;

node * addNode( uint8_t a, uint16_t b){

	node *temp = (node *) malloc( sizeof(node) );
	temp->element = a;
	temp->freq = b;

	return temp;
}

node * makeParent( node * a, node * b){

	node * parent;
	parent = (node *)malloc( sizeof(node) );
	parent->freq = a->freq + b->freq;
	parent->element = 0;
	a->code_bit = 0;
	b->code_bit = 1;
	a->parent_node = b->parent_node = parent;
	return parent;
}

void rearrageArray( node ***array, node *parent, uint16_t *size){

	uint16_t i, j;

	size[0] = size[0] - 1;

	// From right, check till frequency of the element to the left is smaller than/equal to frequency of most recent parent.
	for( i = size[0] ; i >= 0 ; i-- )
		if( ( parent -> freq ) >= ( array[0][i] -> freq ) )
			break;

	// Remove first two elements, move the remaining elements back, add parent.

	for( j = 0 ; j < i-1 ; j++)
		array[0][j] = array[0][j+2];

	array[0][j] = parent;

	while( j < ( size[0] - 1 ) ){
		j++;
		array[0][j] = array[0][j+1];
	}

}

void sortNodeArray ( node ***array, uint16_t length){

	uint16_t i, j;
	node *temp;

	for( i = 0; i < length-1 ; i++ ){
		for( j = 0 ; j < length - i - 1; j++){
			// Bubble Sort.
			if( ( array[0][j] -> freq ) > ( array[0][j+1] -> freq ) ){
				temp = array[0][j];
				array[0][j] = array[0][j+1];
				array[0][j+1] = temp;
			}

		}
	}

}

void getCode( node **array, uint16_t array_size, uint8_t input, uint16_t
*temp_code, uint8_t *length){

	uint16_t i;

	for( i = 0 ; i < array_size ; i++ )
		if( array[i] -> element == input)
			break;

	temp_code[0] = array[i] -> final_code_mask;
	length[0] = array[i] -> code_length;

}

uint16_t huffman(bool quantization_on){
	uint16_t i, j;
	int8_t counter;
	uint16_t *freq_table;
	uint16_t temp_code;
	uint8_t	write_code, unique_count;

	uint16_t write_count, out_count;
	uint16_t array_size, tree_size;
	uint8_t char_read, element_write, length;

	clock_t start_tree, end_tree, start, end;

	node *temp_node_ptr;
	node **node_array, **node_tree_array;

	printf ("Starting Compression.\n");

	// --------------- Compute Frequency Table ------------------------------------------------------------------------------------------------------

	printf("Size of a node = %ld\n", sizeof(node));

	freq_table = ( uint16_t * ) calloc( 256 , sizeof(uint16_t) );
	node_array = ( node ** ) malloc( sizeof(node *) );
	array_size = 0;
	write_count = 0;
	unique_count = 0;
	out_count = 0;
	start_tree = clock();

	// Compute Frequencies of characters read.

	for( i = 0; i < nb; i++ ){
		char_read = frame[i];
		if(quantization_on == true){
				(char_read) = (char_read)/QUANTIZATION_FACTOR;
				(char_read) = (char_read)*QUANTIZATION_FACTOR;
		}
		write_count ++;
		freq_table[char_read]++;
		}

	for( i = 0; i < 256; i++ ){
		if ( freq_table[i] != 0 ){
			unique_count ++;
			// Create a node for each character with respective frequency.
			temp_node_ptr = addNode( (uint8_t) i , freq_table[i] );

			// Add node to array.
			array_size ++;
			node_array = ( node ** ) realloc( node_array, ( array_size * sizeof(node *) ) );
			node_array[array_size-1] = temp_node_ptr;
		}
	}


	// Sort node array in ascending order of frequencies.
	sortNodeArray( &node_array, array_size );

	// --------------- Build Binary Tree ------------------------------------------------------------------------------------------------------

	// Make another copy of node_array

	node_tree_array = ( node ** ) malloc( array_size * sizeof(node *) );
	tree_size = array_size;

	for ( i = 0; i < array_size; i++ )
		node_tree_array[i] = node_array[i];

	#ifdef FREQ_TABLE_PRINT
		printf("Frequency Table:\n");
		for ( i = 0 ; i < tree_size; i++)
			printf( "Element [%d][%p]: %c | Frequency: %d\n",i, node_tree_array[i] , node_tree_array[i]->element, node_tree_array[i]->freq );
	#endif

	while(1){

		temp_node_ptr = makeParent( node_tree_array[0], node_tree_array[1] );
		rearrageArray( &node_tree_array, temp_node_ptr, &tree_size );

		if( tree_size == 1 ){						// Check for root node.
			temp_node_ptr -> parent_node = NULL;
			break;
		}
	}

	// --------------- Build Code Table ------------------------------------------------------------------------------------------------------

	for ( i = 0 ; i < array_size; i++ ){

#ifdef CODE_TABLE_PRINT
		printf("================ PROCESSING ELEMENT : %d ==================\n",node_array[i]->element);
#endif

		length = 1;
		temp_node_ptr = node_array[i];
		temp_code = 0;

		node_array[i] -> final_code_array = ( uint8_t * ) malloc(1);
		node_array[i] -> final_code_array[0] = temp_node_ptr -> code_bit;

		while(1){
			temp_node_ptr = temp_node_ptr -> parent_node;

#ifdef CODE_TABLE_PRINT
		printf("Parent:%d(%p)|%d---->",temp_node_ptr->element,temp_node_ptr,temp_node_ptr->code_bit);
#endif

			if( temp_node_ptr -> parent_node == NULL )
				break;

			length++;

			node_array[i] -> final_code_array = ( uint8_t * )realloc( node_array[i] -> final_code_array, length );
			node_array[i] -> final_code_array[length-1] = temp_node_ptr -> code_bit; 
		}

		node_array[i] -> code_length = length;

		for(j=0; j<length; j++)
			temp_code |= ( (node_array[i] -> final_code_array[length-1-j]) * (uint16_t)pow(2,j) );

		node_array[i] -> final_code_mask = temp_code;

#ifdef CODE_TABLE_PRINT
			printf("\nElement [%d][%p]: %d | Frequency: %d | Code Bit: %d | Parent: %p | Length: %d\n",i, node_array[i] , node_array[i] -> element, node_array[i] -> freq, node_array[i] -> code_bit, node_array[i] -> parent_node, length);
			for(j=0; j<length; j++)
				printf("%d ", node_array[i] -> final_code_array[j]);
			printf("\nCode = %d\n", node_array[i] -> final_code_mask);
#endif
	}

	end_tree = clock();

	// --------------- Encode and Write to File -----------------------------------------------------------------------------------------------------

	start = clock();

	// Write size of code table as the first byte -- LSB first.
	//fwrite( &array_size, 2, 1, op_file );

	frame2[out_count] = array_size & 0xFF;
	out_count++;
	frame2[out_count] = ( array_size >> 8 ) & 0xFF;
	out_count++;

	// Write the code table.
	for ( i = 0 ; i < array_size ; i++ ){
		element_write = node_array[i] -> element;
		length = node_array[i] -> code_length;
		temp_code = node_array[i] -> final_code_mask;

		//printf("Element: %d | Length: %d |  Code : %d\n", element_write, length, temp_code); 
		//fwrite( &element_write, 1, 1, op_file );
		//fwrite( &length, 1, 1, op_file );
		//fwrite( &temp_code, sizeof(unsigned int), 1, op_file );

		// ---- LSB First ----
		frame2[out_count] = element_write;
		out_count++;
		frame2[out_count] = length;
		out_count++;
		frame2[out_count] = temp_code & 0xFF;
		out_count++;
		frame2[out_count] = ( temp_code >> 8 ) & 0xFF;
		out_count++;
	}

	// Write number of total codes to follow.
	// ---- LSB First ----
	// fwrite( &write_count, 1, sizeof(unsigned long int), op_file);
	frame2[out_count] = write_count & 0xFF;
	out_count++;
	frame2[out_count] = ( write_count >> 8 ) & 0xFF;
	out_count++;


	// Encode file
	write_code = 0;
	counter = 8;

	for( i = 0; i < nb; i++){

		char_read = frame[i];

		if(quantization_on == true){
				(char_read) = (char_read)/QUANTIZATION_FACTOR;
				(char_read) = (char_read)*QUANTIZATION_FACTOR;
		}

		getCode( node_array, array_size, char_read, &temp_code, &length);

		#ifdef OR_PRINT
			printf("Char Read: %d | Write code before OR: %d | Temp code before OR:	%d | Length = %d | Counter = %d\n", char_read, write_code, temp_code,	length, counter);
			printf("IN COUNT = %d | OUT COUNT = %d\n", i, out_count);
		#endif

		// Left Shift
		write_code |= (temp_code * (uint16_t)pow(2, (8 - counter)));

		counter -= length;
		if(counter < 0){

			// Write to file.
			// fwrite( &write_code, 4, 1, op_file);
			frame2[out_count] = write_code;
			out_count++;

			#ifdef WRITE_PRINT
				printf("Write Code: %d | Length = %d | Counter = %d\n", write_code,	length, counter);
			#endif

			write_code = 0;

			// Right Shift
			write_code |= (temp_code / (uint16_t)pow(2,(length + counter)));

			// printf("Write Code: %d\n", write_code);

			counter += 8;

			while(counter < 0){
			frame2[out_count] = write_code;
			out_count++;
			write_code = 0;

			// Right Shift
			write_code |= (temp_code / (uint16_t)pow(2,(length + counter)));

			// printf("Write Code: %d\n", write_code);

			counter += 8;
			}
		}
		else if(counter == 0){
			frame2[out_count] = write_code;
			out_count++;

			#ifdef WRITE_PRINT
				printf("Write Code: %d | Length = %d | Counter = %d\n", write_code,	length, counter);
			#endif

			write_code = 0;
			counter += 8;
		}
	}

	// fwrite( &write_code, 4, 1, op_file);
	frame2[out_count] = write_code;
	out_count++;

	end = clock();
	printf("Compression Complete. \nTime taken to build tree: %.3lf seconds\nTime taken to compress file: %.3lf seconds\n", (double) ( (end_tree-start_tree) / CLOCKS_PER_SEC ),(double) ( (end-start) / CLOCKS_PER_SEC ));
	printf("Size of code table = %d\n", array_size);

	free( freq_table );

	return out_count;
}

// ==================== HUFFMAN CODE ENDS HERE ========================


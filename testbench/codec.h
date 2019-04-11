uint16_t nb = 19200;
uint8_t frame2[38400];
uint8_t frame[38400];

uint16_t rle(bool quantization_on);
uint16_t lzw(bool quantization_on);
uint16_t huffman(bool quantization_on);
uint16_t decomp_rle(bool quantization_on, uint16_t input_size);
uint16_t decomp_lzw(bool quantization_on, uint16_t input_size);
uint16_t decomp_huffman(bool quantization_on, uint16_t input_size);

uint16_t file_read( char * input_filename);
uint16_t file_write( char * output_filename, uint16_t write_count);

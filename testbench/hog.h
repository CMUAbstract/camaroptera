uint16_t nb = 19200;
uint8_t frame2[38400];
uint8_t frame[38400];
uint16_t hist8x8[2700];
float hist16x16[9576];

uint16_t file_read( char * input_filename);
uint16_t file_write( char * output_filename, uint16_t write_count);

void sobel(uint8_t height, uint8_t width);
void histogram(uint8_t height, uint8_t width);

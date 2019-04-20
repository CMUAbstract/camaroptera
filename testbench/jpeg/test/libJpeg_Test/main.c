#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "jpec.h"

#define W 160
#define H 120

uint8_t *img;

static int print_usage()
{
   printf("Usage: libJpeg_Test [mode] [source_file] [dest_file] \n");
   printf("mode: 1 = compress, 2 = save .jpg file\n");
   printf("source_file: uint8_t space separed image\n");
   printf("dest_file: Output file.\n");
   printf("\n");
   return 0;
}

static uint16_t read_file(const char *orig){
	
	FILE *pFile;
	long lSize;
	char * buffer;
	size_t result;
	
	pFile = fopen (orig , "r" );
	
	if (pFile==NULL){
		
		fputs ("File error",stderr); 
		system("pause");
		exit(0);
		
	}
		
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);
	
	buffer = (char*) malloc (sizeof(char)*lSize);
	
	if (buffer == NULL) {
	
		fputs ("Memory error", stderr); 
		exit (1);
		
	}
	
	int  j = 0, b, nb = 0;
	
	while (fscanf(pFile, " %d", &b) == 1) {
		nb++;
	}
	
	printf("Found %d bytes in \"%s\" file\n", nb, orig);
	rewind (pFile);
	
	img = (uint8_t*) malloc (sizeof(uint8_t)*nb);
	
	while (fscanf(pFile, " %d", &b) == 1) {
		img[j] = b;
		j++;
	}
	
	fclose(pFile);
	
	return nb;	
}

static void save_img (const char *orig, const char *dest){
	
	FILE *pFile;
	
	uint16_t nb = read_file(orig);
	
	pFile = fopen(dest, "wb");
	fwrite(img, sizeof(uint8_t), nb, pFile);
	fclose(pFile);
	
	printf("Done: saved jpg image to \"%s\" file (%d bytes)\n", dest ,nb);
}

static void compress(const char *orig, const char *dest){
	
	printf("Starting JPEC compression\n\r");
	
	uint16_t nb = read_file(orig);

	jpec_enc_t *e = jpec_enc_new(img, W, H);
	
	int len;
    const uint8_t *jpeg = jpec_enc_run(e, &len);

    printf("JPEC compression done\n\rNew image dimension: %u\n\r", len);
	
	FILE *file = fopen(dest, "w");
	
	for(int i = 0; i < len; i++){
		
		fprintf(file, "%d ", *jpeg);
		jpeg++; 
		
	}
	
    fclose(file);
    
    printf("Done: writed %d bytes to \"%s\" file\n", len, dest);
	    
//    for(int i = 0; i < len; i++){
//
//		printf("%d ", *jpeg);
//		jpeg++;
//		
//	}
	
	/* Release the encoder */
	jpec_enc_del(e);
}

int main(int argc, char *argv[]) {
	
	if(argc != 4){
		
		print_usage();
		system("pause");
		exit(0);
		
	} 
	
	switch(atoi(argv[1])){
		
		case 1:
			compress(argv[2], argv[3]);
			break;
		
		case 2:
			save_img(argv[2], argv[3]);
			break;
			
		default:
			print_usage();
	}
			
	return 0;
}


//	uint16_t i, j, average, total;	
//	for( i = 0; i < 120; i+=2 ){
//	
//		for( j = 0; j < 160; j+=2 ){
//		
//			total = img2[i*160+j] + img2[i*160+(j+1)] + img2[(i+1)*160+j] + img2[(i+1)*160+(j+1)];
//            average = total/4;
//            printf("%d ", average);
//            //img2[((i*160)/4) + j/2] = (uint8_t)average;
//        }
//	}

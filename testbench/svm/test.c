#include <stdio.h>
#include <stdint.h>
#include "svm.h"

int main( int argc, char * argv[] ){

	uint16_t i;
	for( i = 0; i < 9576 ; i++ )
		printf("%.2f\n", svm_w[0][i]); 


	return 0;
	}

#include <stdio.h>
#include <opencv2/opencv.hpp>

#include "display.h"

using namespace cv;

void display()
{
    Mat image;
    image = imread( "Dataset/Divided/Generated_Images/Time0_P0.bmp", 1 );
    if ( !image.data )
    {
        printf("No image data \n");
				exit(0);
		}
    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", image);
    waitKey(0);
}

from PIL import Image
import numpy as np
import os
import sys
import glob
from skimage.feature import hog
from scipy import sqrt, pi, arctan2, cos, sin


input_file = sys.argv[1];
print('Input File: ' + input_file);

path, input_filename = os.path.split(input_file)
print('Path: ' + path)
print('Filename: ' + input_filename)

output_file = "input"
print('Output File: ' + output_file);

#op_flag = input("Do you want to save output to file? (y/n)")

#if op_flag == 'y':
 #   out = open( output_file , 'w') 

def sobel(array, height, width):
    g = [[0 for x in range(width)] for y in range(height)] 
    gx = [[0 for x in range(width)] for y in range(height)] 
    gy = [[0 for x in range(width)] for y in range(height)] 
    angle = [[0 for x in range(width)] for y in range(height)] 

    for i in range(height):
        for j in range(width):
            
            if j == 0 or j == width - 1:
                x_temp = 0
            else:
                x_temp = array[i][j+1] - array[i][j-1]
            
            if i == 0 or i == height - 1:
                y_temp = 0
            else:
                y_temp = array[i+1][j] - array[i-1][j]

            square = (x_temp*x_temp) + (y_temp*y_temp)
            sqrt_temp = sqrt( square )
            g[i][j] = sqrt_temp
            gx[i][j] = x_temp
            gy[i][j] = y_temp

            angle_temp = np.rad2deg(arctan2(y_temp, (x_temp+1e-15))) % 180
            angle[i][j] = angle_temp   
    
    print("Angle " + str(angle))
    print("gx " + str(gx))
    print("gy " + str(gy))
    print("g " + str(g))
    return g, angle

def histo( g, angle, height, width, cell_prows, cell_pcols, block_r, block_c):
    temp = [0 for x in range(9)]
    
    cells_in_x_dir = int(width/cell_pcols)
    cells_in_y_dir = int(height/cell_prows)
    strides_in_x_dir = cells_in_x_dir - block_c + 1
    strides_in_y_dir = cells_in_y_dir - block_r + 1

    hist8x8 = []
    
    for i in range(cells_in_y_dir):
        for j in range(cells_in_x_dir):
            for k in range(cell_prows):
                for l in range(cell_pcols):
                    i1 = i * cell_prows + k
                    j1 = j * cell_pcols + l
                    x = angle[i1][j1] % 20
                    y = angle[i1][j1] / 20
                    lower = int(y % 9)
                    upper = int((y+1) % 9)
                    b = x / 20
                    b = b * g[i1][j1]
                    #temp[upper] += b
                    temp[lower] += g[i1][j1]
            # End of 8x8 loop
            #print(str(temp))
            for k in range(9):  
                hist8x8.append(temp[k])
                temp[k] = 0
            #print(str(temp))
   
    hist8x8 = np.resize(hist8x8,(cells_in_y_dir, cells_in_x_dir, 9))
    print("Histogram 8x8")
    print(str(hist8x8))

    hist16x16 = []

    for i in range(strides_in_y_dir):
        for j in range(strides_in_x_dir):
            sum_var = 0
            for a in range(block_c):
                for b in range(block_r):
                    for k in range(9):
                        sum_var += (hist8x8[i+a][j+b][k] * hist8x8[i+a][j+b][k])
            if sum_var != 0:
                root = sqrt(sum_var)
            else:
                root = 1

            for a in range(block_c):
                for b in range(block_r):
                    for k in range(9):
                        hist16x16.append(hist8x8[i+a][j+b][k]/root)

    
    hist16x16 = np.resize(hist16x16,(strides_in_y_dir, strides_in_x_dir, 9))
    print("Histogram 16x16")
    print(str(hist16x16))

    return hist16x16


def main():
    count = 0
    files = glob.glob("./*.txt")
    
    for input_filename in files:
        file = open(input_filename, 'r')
        string = file.read()
        
        str1 = string.replace(' ', ', ')
        #str1 = list(map(int, str1))
        out = open("headers/input_"+input_filename[2:-4]+".h", "w")
        out.write('uint16_t __attribute__ ((section(".upper.rodata"))) frame[19200] = { ' + str1 + ' };')
    
    
    '''
    #test = [ 1, 1.5, 2, 2.5, 3.835, 4.335, 4.835, 5.335, 6.67, 7.17, 7.67, 8.17, 9.505, 10.005, 10.505, 11.005]
    test = [ 1, 1, -2, -2, 2, 2, 3, 3, 2, 2, 3, 3, 3, 3, -4, -4]
    #test = [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]

    test = np.resize(test, (4,4))
    print(str(test))
    fd, hog_image = hog(test, orientations=9, pixels_per_cell=(2, 2), cells_per_block=(1, 1), block_norm = 'L2', visualize=True, multichannel=False)
    
    #print(str(np.shape(fd)))
    #print(str(fd))

    g, angle = sobel( test, 4, 4 )
    fd1 = histo( g, angle, 4, 4, 2, 2, 1, 1 )
    
    print("\nOG Histogram\n")
    fd = np.resize(fd, (2,2,9))
    print(str(fd))
    #print(str(g))
    #print(str(angle))
    '''
if __name__ == '__main__':
  main()


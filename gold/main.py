import os
import sys
import pickle
import math
import argparse
import numpy as np
import scipy
import skimage
import skimage.feature

from input import frame

F_N = 5
F_ONE = 1 << F_N
to_fixed = lambda x: int(x * F_ONE)
to_float = lambda x: float(x) / F_ONE
flit = np.vectorize(to_fixed)
floatlit = np.vectorize(to_float)

np.set_printoptions(precision=3, suppress=True, threshold=sys.maxsize)

def atan_lookup(x):
	if x >= 0.0 and x < 0.36: return 0
	elif x >= 0.36 and x < 0.84: return 1
	elif x >= 0.84 and x < 1.73: return 2
	elif x >= 1.73 and x < 5.67: return 3
	elif x < -5.67 or x >= 5.67: return 4
	elif x >= -5.67 and x < -1.73: return 5
	elif x >= -1.73 and x < -0.84: return 6
	elif x >= -0.84 and x < -0.36: return 7
	elif x >= -0.36 and x < 0: return 8
	return 0;

def sobel(array, height, width):
	g = [[0 for x in xrange(width)] for y in xrange(height)] 
	angle = [[0 for x in xrange(width)] for y in xrange(height)] 
	angle_idx = [[0 for x in xrange(width)] for y in xrange(height)] 

	for i in xrange(1, height - 1):
		for j in xrange(1, width - 1):
			x_temp = array[i][j+1] - array[i][j-1]
			
			y_temp = array[i+1][j] - array[i-1][j]

			square = (x_temp * x_temp) + (y_temp * y_temp)
			sqrt_temp = scipy.sqrt(square)
			g[i][j] = sqrt_temp

			angle_temp = np.rad2deg(scipy.arctan2(y_temp, (x_temp+1e-15))) % 180
			angle[i][j] = angle_temp
			angle_idx[i][j] = atan_lookup(y_temp / (x_temp + 1e-15))
	
	return g, angle, angle_idx
	
def histo(g, angle, height, width, cell_prows, cell_pcols, block_r, block_c):
	temp = [0 for _ in xrange(9)]
	
	cells_in_x_dir = int(width/cell_pcols)
	cells_in_y_dir = int(height/cell_prows)
	strides_in_x_dir = cells_in_x_dir - block_c + 1
	strides_in_y_dir = cells_in_y_dir - block_r + 1

	hist8x8 = []
	
	for i in xrange(cells_in_y_dir):
		for j in xrange(cells_in_x_dir):
			for k in xrange(cell_prows):
				for l in xrange(cell_pcols):
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
			for k in xrange(9):  
				hist8x8.append(temp[k])
				temp[k] = 0
			#print(str(temp))

	hist8x8 = np.resize(hist8x8,(cells_in_y_dir, cells_in_x_dir, 9))

	hist16x16 = []

	for i in xrange(strides_in_y_dir):
		for j in xrange(strides_in_x_dir):
			sum_var = 0
			for a in xrange(block_c):
				for b in xrange(block_r):
					for k in xrange(9):
						sum_var += (hist8x8[i+a][j+b][k] * hist8x8[i+a][j+b][k])
			if sum_var != 0:
				root = math.sqrt(sum_var)
			else:
				root = 1

			for a in xrange(block_c):
				for b in xrange(block_r):
					for k in xrange(9):
						if i == 1 and j == 0:
							print hist8x8[i+a][j+b][k], root
						hist16x16.append(hist8x8[i+a][j+b][k]/root)

	
	hist16x16 = np.resize(hist16x16,(strides_in_y_dir, strides_in_x_dir, 9))

	return hist8x8, hist16x16

def main(args):
	data = np.asarray(frame).reshape((120, 160))
	data = data.astype(float)
	gx = skimage.filters.sobel_h(data)
	gy = skimage.filters.sobel_v(data)

	g = np.sqrt(gx * gx + gy * gy)
	theta = np.degrees(np.arctan(gy / (gx + 1e-9)))
	theta[theta < 0] += 180

	hogs = skimage.feature.hog(data, orientations=9, pixels_per_cell=(8, 8), 
		cells_per_block=(2, 2), block_norm='L2', multichannel=False)
	g, theta, theta_idx = sobel(data, 120, 160)
	hist8x8, hist16x16 = histo(g, theta, 120, 160, 8, 8, 2, 2)
	g = np.asarray(g).reshape((120, 160))
	theta = np.asarray(theta).reshape((120, 160))
	theta_idx = np.asarray(theta_idx).reshape((120, 160))
	print g[:16,:16].astype(int)
	print theta_idx[:16,:16]
	# print hist8x8.shape
	# print hist16x16.shape
	print hist8x8[:15,:1]
	print flit(hist16x16)[:14,:1]

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--header_dir',
		type=str,
		default='headers',
		help='Generated Header(s) Directory')
	args = parser.parse_args()
	main(args)
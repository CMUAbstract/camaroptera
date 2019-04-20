import os
import pickle
import math
import argparse
import numpy as np
import skimage

from input import frame

F_N = 5
F_ONE = 1 << F_N
to_fixed = lambda x: int(x * F_ONE)
to_float = lambda x: float(x) / F_ONE
flit = np.vectorize(to_fixed)
floatlit = np.vectorize(to_float)

np.set_printoptions(precision=3, suppress=True)

def main(args):
	path = os.path.join(args.param_dir, 'data.param')
	# if not os.path.exists(path):
	# 	data = np.random.randint(-5, 5, size=(8, 8))
	# 	data[data == 1] = 0
	# 	data[data == -1] = 0
	# 	with open(path, 'w+') as f:
	# 		pickle.dump(data, f)
	# else:
	# 	with open(path, 'r') as f:
	# 		data = pickle.load(f)

	data = np.asarray(frame).reshape((120, 160))
	data = data.astype(float)
	gx = skimage.filters.sobel_h(data)
	gy = skimage.filters.sobel_v(data)

	g = np.sqrt(gx * gx + gy * gy)
	theta = np.degrees(np.arctan(gy / (gx + 1e-9)))
	theta[theta < 0] += 180

	print g[:16,:16]
	# print theta[:16,:16].astype(int)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--header_dir',
		type=str,
		default='headers',
		help='Generated Header(s) Directory')
	parser.add_argument(
		'--param_dir',
		type=str,
		default='params',
		help='Checkpoint Directory')
	args = parser.parse_args()
	main(args)
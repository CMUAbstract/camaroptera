import argparse
from PIL import Image
import numpy as np

WIDTH = 80
HEIGHT = 60 

def main(args):

	with open( args.raw_file, 'r' ) as input_file:
		lines = input_file.readlines()
	
	for line in lines:
		raw_bytes = list(map(int, line.split(' ')))
		print(len(raw_bytes))
	
	arr = np.reshape( raw_bytes, (HEIGHT, WIDTH)).astype(dtype=np.uint8)

	img = Image.fromarray(arr)
	
	output_name = args.raw_file.split('.')[0]
	
	img.save(output_name+".bmp")

if __name__ == '__main__':
	parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

	parser.add_argument(
		'--raw_file',
		type=str,
		help='File containing raw image bytes')
	args = parser.parse_args()
	main(args)

import argparse
from PIL import Image
import numpy as np

JPEG_QF = 95

WIDTH = 160
HEIGHT = 120

def process(args):
	with open( args.raw_file, 'r' ) as input_file:
		lines = input_file.readlines()
	
	for line in lines:
		raw_bytes = list(map(int, line.split(' ')))
		print(len(raw_bytes))


	return raw_bytes 

def load_jpeg_header():
   global JPEG_HEADER
   ip_file = open("utils/jpeg_header_QF%s.txt" % JPEG_QF)
   header = ip_file.read()
   header = header.split(' ')
   JPEG_HEADER = list(map(int, header))

def main(args):

	load_jpeg_header()
	data = process(args)	
	
	image = JPEG_HEADER + data
	bt = bytes(image)
	fp = open(args.op, 'wb')
	fp.write(bt)
	fp.close()

if __name__ == '__main__':
	parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

	parser.add_argument(
		'--op',
		type=str,
		default='stream.jpg',
		help='File to store image in')
	parser.add_argument(
		'--raw_file',
		type=str,
		help='Input file containing headerless jpeg bytes as ascii')
	args = parser.parse_args()
	main(args)


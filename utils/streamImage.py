import serial
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
	
	arr = np.reshape( raw_bytes, (HEIGHT, WIDTH)).astype(dtype=np.uint8)

	img = Image.fromarray(arr)
	
	output_name = args.raw_file.split('.')[0]
	
	img.save(output_name+".jpg")

def load_jpeg_header():
   global JPEG_HEADER
   ip_file = open("jpeg_header_QF%s.txt" % JPEG_QF)
   header = ip_file.read()
   header = header.split(' ')
   JPEG_HEADER = list(map(int, header))

def main(args):

	load_jpeg_header()
	
	# Open Serial Link
	#print(args.input_device)
	ser = serial.Serial('COM5', baudrate=9600)
	print(ser.name)			# Verify correct device opened
	
	flag = 0
	
	datanext = False 
	# Run forever
	while True:
		if ser.is_open == False:
			ser.open()
		
		# Read one line from serial
		line = ser.readline()
		#print(line) 
		if line == b"Received Message:\r\n":		   # Check if this is a received message
			datanext = True
		if datanext == True:
			print("")
			print(line.decode('utf-8'))
			data = ser.readline().decode('utf-8')
			data = data.split(' ')[5:-2]
			data = [ int(x) for x in data ]
			print('Captured %s bytes' % len(data))
			print(data)
		
			#image = JPEG_HEADER + data
			#bt = bytes(image)
			#fp = open(args.op, 'wb')
			#fp.write(bt)
			#fp.close()
		
		else:
			pass
		



if __name__ == '__main__':
	parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

	parser.add_argument(
		'--op',
		type=str,
		default='stream.jpg',
		help='File to store image in')
	parser.add_argument(
		'--input_device',
		type=str,
		help='Selected Serial Device')
	args = parser.parse_args()
	main(args)


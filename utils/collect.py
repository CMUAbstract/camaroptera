import os
import re
import glob
import serial
import argparse
import numpy as np
from PIL import Image

WIDTH = 160
HEIGHT = 120

def main(args):
	img_count = 0
	
	files = list(glob.glob(f'{args.dest}/*'))
	for file in files:
		if 'frame' not in file: continue
		s = re.findall('\d+', os.path.basename(file))
		img_count = max(img_count, int(s[0])) + 1

	print(f'[DEBUG] Starting @ {img_count:05d}')

	with serial.Serial(args.port, baudrate=args.baud) as ser:
		while True:
			line = ser.readline()
			if b'Start captured frame' in line:
				data = ''
				while True:
					line = ser.readline()
					if b'End frame' in line:
						break

					data += line.decode('utf-8')

				mat = list(map(int, data.strip().split(' ')))
				mat = np.array(mat, dtype=np.uint8)
				mat = mat.reshape(HEIGHT, WIDTH)
				img = Image.fromarray(mat)
				print(f'[DEBUG] Dumping frame: {img_count:05d}')
				img.save(f'{args.dest}/frame{img_count:05d}.png')
				img_count += 1

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'-p', '--port',
		type=str,
		help='Port')
	parser.add_argument(
		'-b', '--baud',
		type=int,
		default=115200,
		help='baud')
	parser.add_argument(
		'-d', '--dest',
		type=str,
		help='Destination')
	args = parser.parse_args()
	main(args)
import os
import re
import csv
import glob
import serial
import argparse
import numpy as np
import math

VCC = 3.3 #3.3 for serial/usb power, 3 o/w
ADC_BIT_DEPTH = 12 #Using ADC12
ADC_RANGE = math.pow(2,ADC_BIT_DEPTH)
CAL_TIME = 0.996 #Calibrate using bit flip script/saleae (ms)


def main(args):
	samp_count = 0

	print(f'[DEBUG] Starting Power Read')

	with serial.Serial(args.port, baudrate=args.baud) as ser:
		while True:
			data = ser.readline().decode('utf-8').replace("\r\n", "")
			if data.isdigit():
				conv_data = (int(data) * VCC)/ADC_RANGE
				with open(args.file, 'a') as file:
					writer = csv.writer(file, delimiter = '\t')
     
					writer.writerow([round(samp_count*CAL_TIME,3), round(conv_data,3)])
					file.close()
				samp_count += 1

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
		'-f', '--file',
		type=str,
		help='Out File')
	args = parser.parse_args()
	main(args)

import os
import re
import csv
import glob
import math
import serial
import argparse
import numpy as np
from decimal import Decimal
import datetime

# =============== MUST CONNECT RSTSENSE TO GND ======================

VCC = 3.2726 							#3.2726 for serial/usb power, 3 o/w
ADC_BIT_DEPTH = 12 						#Using ADC12
ADC_RANGE = math.pow(2,ADC_BIT_DEPTH)
CAL_TIME = 0.996 						#Calibrate using bit flip script/saleae (ms)

READ_INTERVAL = 1 						#Interval must be an integer, reading taken every READ_INTERVAL milliseconds

COLLECTION_WINDOW = 10.0					#sample collection window specified in seconds

SC_MF = 33 								#SuperCapacitor millifarrad Specification

def main(args):
	last_data = 0
	power = 0
	dvdt = 0

	start_time = datetime.datetime.now()
	end_time = datetime.datetime.now()
	

	datas = []

	print(f'[DEBUG] Starting Power Read')
	with serial.Serial(args.port, baudrate=args.baud) as ser:
		start_time = datetime.datetime.now()
		data_time = start_time
		end_time = start_time + datetime.timedelta(0,COLLECTION_WINDOW)
		while (data_time < end_time):
			data_time = (datetime.datetime.now())
			data = ser.readline().decode('utf-8').replace("\r\n", "")
			if data.isdigit():
				datas.append((data_time, int(data)))
				
	print(f'[DEBUG] Power Read Finished, writing to outfile')
	
	with open(args.file, 'a') as file:
		writer = csv.writer(file, delimiter = '\t')
		writer.writerow(["Calculated Time (From calibrated clock)", "Recorded Time (ms)", "Voltage (V)", "dV/dt", "Power (mW)"])
		for i, (data_time, data) in enumerate(datas):
			conv_data = (data * VCC)/ADC_RANGE
			if(i!=0):
				dvdt = ((conv_data - last_data) / (CAL_TIME/1000) ) 
				cap_i = SC_MF * dvdt
				power = cap_i * conv_data
			if(i % READ_INTERVAL == 0):
					writer.writerow([round(i*CAL_TIME,3), round((data_time-start_time).total_seconds()*1000,3), round(conv_data,3), round(dvdt, 3), round(power, 3)])
			last_data = conv_data
		file.close()
	print(f'[DEBUG] Writing to outfile finished')

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

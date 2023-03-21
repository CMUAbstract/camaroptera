import serial
import argparse
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import datetime
import os

JPEG_QF = 95

PACKET_SIZE = 255
IMAGE_SIZE = 160*120

SIZE_OF_DEVICE_ID_IN_BITS = 8
MAX_DEVICES = 2**SIZE_OF_DEVICE_ID_IN_BITS

MAC_HEADER = 0xDF
HEADER_SIZE = 6

font = ImageFont.truetype('Inconsolata.otf', size=45)
color = 'rgb(0, 0, 0)' # black color
color1 = 'rgb(12, 105, 93)' 

image_path = './latest'
history_path = './history'

server_path = '' 

class frame:
	
	def __init__(self):
		self.frame_id = 0
		self.packets_for_full_frame = 0
		self.packet_data = None
		self.last_packet_length = 0

	def update_parameters(self, frame_id, packets_for_full_frame):
		self.frame_id = frame_id
		self.packets_for_full_frame = packets_for_full_frame
		self.packet_data = np.zeros((self.packets_for_full_frame, PACKET_SIZE-HEADER_SIZE)).astype(dtype=int)
		self.last_packet_length = PACKET_SIZE


class device:
	
	def __init__(self, dev_id):
		self.dev_id = dev_id
		self.packets_recd_count = 0				# Used only for local stats - computing packet loss, tracking which packets were lost, etc
		self.active_frame = False				# True: Is in the middle of receiving packets for a frame | False: Last frame completed, next frame yet to start
		self.frame = frame()
		self.latest_frame_id = 0
		self.latest_packet_rssi = 0
		self.latest_packet_snr = 0
		self.image_name = os.path.join( image_path, 'trailcam_%s.jpg' % self.dev_id )
		self.rssi_name = os.path.join( image_path, 'trailcam_%s_rssi.png' % self.dev_id )

	def increment_packet_count(self):
		self.packets_recd_count += 1 
	
	def add_frame(self, frame_id, packets_for_full_frame):
		self.increment_packet_count()
		self.frame.update_parameters(frame_id, packets_for_full_frame)
		self.latest_frame_id = frame_id

	def save_and_display_frame(self):
		self.frame.packet_data = self.frame.packet_data.flatten()
		final_len = ((self.frame.packets_for_full_frame - 1) * (PACKET_SIZE-HEADER_SIZE)) + self.frame.last_packet_length
		self.frame.packet_data = self.frame.packet_data[:final_len]
		print("Flattened shape: " + str(np.shape(self.frame.packet_data)))
		self.save_to_file()
		self.save_latest_packet_stats()
		self.display_image()				
		#self.display_to_webpage()

	def delete_frame(self):
		self.frame.__init__()

	def print_stats(self):
		print("Packets received: %s" % self.packets_recd_count)
		print("Packets supposed to have received: %s" % self.latest_frame_id)
		print("Packets miss rate: %s" % 100*((self.latest_frame_id - self.packets_recd_count)/self.latest_frame_id))
	
	def print_latest_packet_stats(self):
		print("RSSI: %s" % self.latest_packet_rssi)
		print("SNR: %s" % self.latest_packet_snr)
   
	def save_latest_packet_stats(self):
		# save rssi, snr and timestamp to image file
		arr = np.zeros([165,1000,3],dtype=np.uint8)
		arr.fill(255)
		image = Image.fromarray(arr)
		draw = ImageDraw.Draw(image)
		date_message = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
		# draw the message on the background
		draw.text((25, 50), "Time of capture:", fill=color, font=font)
		draw.text((450, 50), date_message, fill=color1, font=font)
		draw.text((25, 100), "RSSI:", fill=color, font=font)
		draw.text((150, 100), str(self.latest_packet_rssi), fill=color1, font=font)
		draw.text((275, 100), "| SNR:", fill=color, font=font)
		draw.text((450, 100), str(self.latest_packet_snr), fill=color1, font=font)
		image.save(self.rssi_name) 
	
	def save_to_file(self):
		# save raw jpeg bytes as a .jpg file
		# decode jpeg and save raw bytes?
		self.frame.packet_data = list(self.frame.packet_data)
		self.frame.packet_data = JPEG_HEADER + self.frame.packet_data
		bt = bytes(self.frame.packet_data)
		op = open(self.image_name, 'wb')
		print("Wrote %s bytes to file." % op.write(bt))

		date_message = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M")
		hist_image_name = os.path.join( history_path, 'trailcam_%s_%s.jpg' % (self.dev_id, date_message) )
		op = open(hist_image_name, 'wb')
		op.write(bt)
		op.close()
		
		
	def display_image(self):
		# split screen to accomodate every distinct device
		'''
		picture = Image.Image(self.packet_data)
		width, height = picture.size()
		pix = picture.getPixels()
		print("width:%s | height:%s" % (width, height))
		print("pixels:%s" % pix) 
		'''
		#ip = Image.open('latest.jpg', 'L')
		#Image.show()
		pass

	def display_to_webpage(self):
		# display to screen over webpage?
		os.system("scp %s %s %s", self.image_name, self.rssi_name, server_path)		

devices = [device(i) for i in range(MAX_DEVICES)]

def check_if_device_has_frame_active( dev_id ):
	# Function to check if bit set in the [dev_id] field of the device_with_active_frame array
	# Can be replaced with a hash table to reduce exponential size growth of device_with_active_frame? -- Currently trivially implemented

	return devices[dev_id].active_frame

def parse_packet_information( data, length, rssi, snr ):
	
	if length < HEADER_SIZE:
		print("ERROR: Packet length smaller than minimum required to decode.")
		return False
	
	mac_hdr = data[0]
	if mac_hdr != MAC_HEADER :							# Confirm if this comparison is on raw hex byte or on string
		print("ERROR: Incorrect MAC Header.")
		return False
	
	dev_id = data[1]
	frame_id = data[2]
	total_packets = data[3]
	packet_id = data[4]
   
	print("Received packet of size %s from Device with dev_id: %s" % (length, dev_id))

	if check_if_device_has_frame_active(dev_id) == False:			# Condition triggers if device has no frame partially received

		devices[dev_id].active_frame = True				# devices[dev_id] is an object of class device
		
		# Initialize frame object with frame_id, packets_for_full_frame
		devices[dev_id].add_frame(frame_id, total_packets)
		print("This is a new frame")
	else:															# Condition triggers if device has frame partially received
		
		if frame_id != devices[dev_id].frame.frame_id:	# if the frame received is different from the frame we're currently constructing
			devices[dev_id].save_and_display_frame()			# save and display previous partial image to file
			devices[dev_id].add_frame(frame_id, total_packets)	# add new frame to data struct
			print("This is a new frame")

	print("Frame ID: %s" % frame_id)
	print("This frame is made up of %s packets" % total_packets)
	print("This packet is %s out of %s packets" % (packet_id+1, total_packets))
	print("New shape of packet_data: " + str(np.shape(devices[dev_id].frame.packet_data)))
	#print("Packet Data: %s" % devices[dev_id].frame.packet_data[packet_id])

	# Put data from data[HEADER_SIZE] to data[length-1] into devices[dev_id]->frame[frame_id]->packet_data[packet_id][0 to length-HEADER_SIZE-1] 
	for i in range(length-HEADER_SIZE):
		devices[dev_id].frame.packet_data[packet_id][i] = data[HEADER_SIZE+i] 
	
	devices[dev_id].latest_packet_rssi = rssi
	devices[dev_id].latest_packet_snr = snr
	devices[dev_id].print_latest_packet_stats()
	
	# save and display image if last packet of frame
	if packet_id == devices[dev_id].frame.packets_for_full_frame - 1:
		devices[dev_id].frame.last_packet_length = length - HEADER_SIZE
		devices[dev_id].save_and_display_frame()
		# print("Packet Data: %s" % devices[dev_id].frame.packet_data)
		devices[dev_id].delete_frame()
		devices[dev_id].active_frame = False
	
def load_jpeg_header():
   global JPEG_HEADER
   ip_file = open("jpeg_header_QF%s.txt" % JPEG_QF)
   header = ip_file.read()
   header = header.split(' ')
   JPEG_HEADER = list(map(int, header))

def main(args):
	
	load_jpeg_header()
	
	# Open Serial Link
	print(args.input_device)
	ser = serial.Serial(args.input_device, baudrate=9600)
	print(ser.name)			# Verify correct device opened
	
	flag = 0
	
	if not os.path.exists(image_path):
		os.makedirs(image_path)
	if not os.path.exists(history_path):
		os.makedirs(history_path)

	# Run forever
	while True:
	   
		if ser.is_open == False:
			ser.open()
		
		# Read one line from serial
		line = ser.readline()
		#print(line) 
		if line == b">OnRxDone\r\n":		   # Check if this is a received message
			print("")
			print(line.decode('utf-8'))
		elif line == b'Received Message:\r\n':
			data = ser.readline().decode('utf-8')
			#print(data)
		elif line == b'Message Length:\r\n':
			length = int(ser.readline().decode('utf-8'))
			#print(length)
		elif line == b'RSSI:\r\n':
			rssi = int(ser.readline().decode('utf-8'))
			#print(rssi)
		elif line == b'SNR:\r\n':
			snr = int(ser.readline().decode('utf-8'))
			#print(snr)
			flag = 1
		elif line == b">OnRxError\r\n":			  # Check if this is an error message
			print("")
			print(line.decode('utf-8'))
		else:
			pass
 
		if flag == 1:
			print("This is a valid packet")
			data = data.split(',')[:-1]
			data = list(map(int, data))
			if len(data) != length:
				flag = 0
			else:
				#print("SPLIT DATA Length: %s" % len(data) )
				#print(hex(data[0]))
				parse_packet_information( data, length, rssi, snr )
				flag = 0
			ser.close()
		else:
			pass


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--input_device',
		type=str,
		help='Selected Serial Device')
	args = parser.parse_args()
	main(args)

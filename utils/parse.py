import os
import pickle
import argparse
import numpy as np

CLASSES = {
	0 : 'NOPE',
	1 : 'DINOSAUR'
}

def load_jpeg_header():
	global JPEG_HEADER
	ip_file = open("utils/jpeg_header_QF%s.txt" % JPEG_QF)
	header = ip_file.read()
	header = header.split(' ')
	JPEG_HEADER = list(map(int, header))

def main(args):
	imgs = {}
	with open(args.src, 'rb') as f:
		pkts = pickle.load(f)
		for pkt in pkts:
			mac_hdr = pkt[0]
			dev_id = pkt[1]
			frame_index = pkt[2]
			packet_count = pkt[3]
			tx_packet_index = pkt[4]
			prediction = pkt[5]
			data = pkt[6:]

			if frame_index not in imgs:
				imgs[frame_index] = {
					'pkts' : {},
					'prediction' : prediction
				}

			if tx_packet_index not in imgs[frame_index]:
				imgs[frame_index]['pkts'][tx_packet_index] = data
				
	for frame in imgs:
		img = []
		pkts = imgs[frame]['pkts']
		if imgs[frame]['prediction'] not in CLASSES: continue
		
		prediction = CLASSES[imgs[frame]['prediction']]
		for pkt in sorted(pkts.keys()):
			img += imgs[frame]['pkts'][pkt]

		img = np.asarray(img, dtype=np.uint8)

		print(f'[DEBUG] Frame {frame}')
		print(f'Predicted: {prediction}')
		print(img)
	
		if args.jpeg:
			with open(args.jpeg, 'r') as f:
				header = f.read()
				header = header.split(' ')
				header = list(map(int, header))

			bt = bytes(header) + bytes(img)
			if args.dest:
				path = os.path.join(args.dest, f'frame{frame:05d}.jpg')
				with open(path, 'wb+') as f:
					f.write(bt)	

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'-d', '--dest',
		type=str,
		help='Destination')
	parser.add_argument(
		'--jpeg',
		type=str,
		help='Jpeg header file')
	parser.add_argument(
		'src',
		type=str,
		help='Source')
	args = parser.parse_args()
	main(args)
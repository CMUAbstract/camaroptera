import pickle
import signal
import sys
import serial
import argparse

interrupted = False;

def signal_handler(sig, frame):
	global interrupted
	interrupted = True;

def main(args):
	global interrupted
	pkts = []
	
	signal.signal(signal.SIGINT, signal_handler)

	with serial.Serial(args.port, baudrate=args.baud) as ser:
		while not interrupted:
			line = ser.readline()
			if b'Received Message' in line:
				pkt = ''
				while not interrupted:
					line = ser.readline()
					if b'Message Length' in line:
						break

					pkt += line.decode('utf-8').strip()

				pkt = filter(lambda x: len(x) > 0, pkt.split(','))
				pkt = list(map(int, pkt))
				frame_index = pkt[2]
				tx_packet_index = pkt[4]
				pkts.append(pkt)
				print(f'[DEBUG] Recieved {frame_index} {tx_packet_index}: {pkt}')
				
	if args.dest:
		with open(args.dest, 'wb+') as f:
			print(f'[DEBUG] Dumping to {args.dest}')
			pickle.dump(pkts, f)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'-p', '--port',
		type=str,
		help='Port')
	parser.add_argument(
		'-b', '--baud',
		type=int,
		default=9600,
		help='baud')
	parser.add_argument(
		'-d', '--dest',
		type=str,
		help='Destination')
	args = parser.parse_args()
	main(args)
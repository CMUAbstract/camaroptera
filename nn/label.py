import os
import re
import glob
import random
import argparse
import subprocess

ZSH = '/usr/local/bin/zsh'

def execute(cmd):
	global ZSH
	p = subprocess.Popen(cmd, shell=True, executable=ZSH, 
		stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	return out, err

def mkdir_p(path):
	cmd = 'mkdir -p %s' % os.path.dirname(path)
	return execute(cmd)

def cp(path, dest):
	cmd = 'cp %s %s' % (path, dest)
	return execute(cmd)

def main(args):
	if args.dest is None:
		print('No destination supplied')
		return

	files = list(glob.glob(f'{args.raw}/*'))
	if args.shuffle:
		random.shuffle(files)

	split = args.split.split(',')
	parse_split = lambda x: int(len(files) * int(x) / 100)
	train_len, val_len, test_len = map(parse_split, split)
	train = files[:train_len]
	val = files[train_len:train_len + val_len]
	test = []
	if (train_len + val_len) < len(files):
		test = files[train_len + val_len:]

	for typ, files in [('train', train), ('val', val), ('test', test)]:
		dest = '%s/%s/' % (args.dest, typ)
		mkdir_p(dest)
		label_path = os.path.join(dest, 'labels.csv')
		with open(label_path, 'w+') as f:
			for file in sorted(files):
				label = ''
				# label = 'LABEL'
				# label = '0' if random.random() < 0.5 else '1'
				cp(file, dest)
				f.write('%s,%s\n' % (os.path.basename(file), label))

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'raw',
		type=str,
		help='Raw data source')
	parser.add_argument(
		'--shuffle',
		action='store_true',
		help='Shuffle')
	parser.add_argument(
		'--split',
		type=str,
		default='80,15,5',
		help='Split train,val,test')
	parser.add_argument(
		'--dest',
		type=str,
		help='Destination directory')
	args = parser.parse_args()
	main(args)
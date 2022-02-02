import os
import csv
import glob
import random
import argparse
import subprocess

from tqdm import tqdm
from PIL import Image, ImageEnhance, ImageFilter

ZSH = '/usr/local/bin/zsh'

def execute(cmd):
	global ZSH
	p = subprocess.Popen(cmd, shell=True, executable=ZSH, 
		stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	return out, err

def mkdir_p(path):
	return execute(f'mkdir -p {path}')

def cp(path, dest):
	return execute(f'cp {path} {dest}')

def fliph(img):
	return img.transpose(Image.FLIP_LEFT_RIGHT)

def flipv(img):
	return img.transpose(Image.FLIP_TOP_BOTTOM)

def bright(img):
	f = (random.random() * 0.2) + 0.9
	enhancer = ImageEnhance.Brightness(img)
	return enhancer.enhance(f)

def contrast(img):
	f = (random.random() * 0.2) + 0.9
	enhancer = ImageEnhance.Contrast(img)
	return enhancer.enhance(f)

def sharpness(img):
	f = (random.random() * 0.2) + 0.9
	enhancer = ImageEnhance.Sharpness(img)
	return enhancer.enhance(f)

def blur(img):
	f = 1 if random.random() < 0.5 else 2
	return img.filter(ImageFilter.GaussianBlur(f))

def scale(img):
	w, h = img.size
	img = img.resize((w * 2, h * 2))

	width, height = img.size
	left = round((width - w)/2)
	top = round((height - h)/2)
	x_right = round(width - w) - left
	x_bottom = round(height - h) - top
	right = width - x_right
	bottom = height - x_bottom

	return img.crop((left, top, right, bottom))

transforms = [flipv, fliph, bright, contrast, sharpness, blur, scale]

def main(args):
	if args.dest_data is None:
		print('No destination supplied for data')
		return

	if args.dest_lbls is None:
		print('No destination supplied for labels')
		return

	mkdir_p(args.dest_data)
	files = list(glob.glob(f'{args.raw_data}/*'))
	labels = {}
	with open(args.raw_lbls, 'r') as f:
		reader = csv.reader(f)
		for row in reader:
			name, lbl = row
			lbl = int(lbl)
			if lbl < 0:
				continue
			labels[name] = lbl

	count = len(files)
	for file in tqdm(files):
		name = os.path.basename(file)
		fname, ext = name.split('.')
		if name not in labels:
			continue

		cp(file, args.dest_data)
		lbl = labels[name]

		tqdm.write(f'[DEBUG] Augmenting {file} : {lbl}')

		for idx in range(args.factor-1):
			l = random.randint(1, len(transforms))
			transform = random.sample(transforms, l)
			img = Image.open(file)
			for t in transform:
				img = t(img)

			new_name = f'{fname}_{idx}.{ext}'
			path = os.path.join(args.dest_data, new_name)
			img.save(path)
			labels[new_name] = lbl

	with open(args.dest_lbls, 'w+') as f:
		for file, label in labels.items():
			f.write(f'{file},{label}\n')	

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--factor',
		type=int,
		default=2,
		help='Multiplicative factor')
	parser.add_argument(
		'--dest-lbls',
		type=str,
		help='Raw data labels')
	parser.add_argument(
		'--dest-data',
		type=str,
		help='Raw data source')
	parser.add_argument(
		'--raw-lbls',
		type=str,
		help='Raw data labels')
	parser.add_argument(
		'--raw-data',
		type=str,
		help='Raw data source')
	args = parser.parse_args()
	main(args)
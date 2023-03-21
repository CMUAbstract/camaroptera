'''
The main function of this file
'''
import os
import glob
import argparse
import subprocess
import tkinter as tk

from PIL import Image
from PIL import ImageTk as itk
from PIL import ImageFile

ZSH = '/usr/local/bin/zsh'
CLASSES = {
	0 : 'Nope',
	1 : 'Yellow Dino',
	2 : 'TRex'
}

def execute(cmd):
	global ZSH
	p = subprocess.Popen(cmd, shell=True, executable=ZSH, 
		stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	return out, err

def mv(src, dst):
	return execute(f'mv {src} {dst}')

def mkdir_p(path):
	return execute(f'mkdir -p {path}')

idx = 0
def handler_factory(window, labels, label, files):
	def handler(event):
		global idx
		if event.keysym.isdigit():
			labels[files[idx]] = int(event.keysym)
			idx += 1
		elif event.keysym == 'u' or event.keysym == 'p':
			idx = max(0, idx - 1)
		elif event.keysym == 'n':
			idx += 1
		elif event.keysym == 'q':
			idx = len(files)
		elif event.keysym == 'x':
			print(f'[DEBUG] Deleting {files[idx]}')
			labels[files[idx]] = -1
			idx += 1

		if idx == len(files):
			window.destroy()
			return

		file = files[idx]
		percent_done = (len(labels) / len(files)) * 100
		print(f'[DEBUG] Showing {file} Labeled: {percent_done:.1f}%')
		if file in labels:
			lbl = labels[file]
			if lbl in CLASSES:
				label_text = CLASSES[lbl]
			else:
				label_text = 'class not recognized'
		else:
			label_text = 'unlabeled'

		img = Image.open(file)
		w, h = img.size
		img = img.resize((w * 2, h * 2))
		img = itk.PhotoImage(img)
		label.configure(image=img, text=label_text)
		label.img = img

	return handler

def main(args):
	files = list(glob.glob(f'{args.raw}/*'))	
	window = tk.Tk()
	frame = tk.Frame(window)
	frame.pack();
	label = tk.Label(frame, compound='bottom')
	label.pack()

	file = files[0]
	print(f'[DEBUG] Showing {file}')
	img = Image.open(file)
	w, h = img.size
	img = img.resize((w * 2, h * 2))
	img = itk.PhotoImage(img)
	label.configure(image=img, text='unlabeled')

	labels = {}
	window.bind('<KeyPress>', handler_factory(window, labels, label, files))
	window.mainloop()

	if args.dest:
		with open(args.dest, 'w+') as f:
			for file, label in labels.items():
				file = os.path.basename(file)
				f.write(f'{file},{label}\n')
		return

	print(labels)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'raw',
		type=str,
		help='Raw data source')
	parser.add_argument(
		'--dest',
		type=str,
		help='Destination csv')
	args = parser.parse_args()
	main(args)
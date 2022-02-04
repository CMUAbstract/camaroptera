import torch
import argparse
import torch.nn as nn
import numpy as np

from tqdm import tqdm
from datetime import datetime
from torch.utils.data import DataLoader
from torchvision import transforms

from globals import *
from lenet import *
from compress import QUANTIZERS_TORCH as QUANTIZERS
from compress import SPARSIFIERS_TORCH as SPARSIFIERS

def main(args):
	device = 'cuda' if torch.cuda.is_available() else 'cpu'

	transform = transforms.Compose([
		transforms.Resize((HEIGHT, WIDTH)),
		transforms.ToTensor()])
	
	quantizer = QUANTIZERS[args.quantize] if args.quantize else None
	sparsifier = SPARSIFIERS[args.sparsify] if args.sparsify else None

	model = LeNet(CLASSES, quantizer, sparsifier).to(device)

	if args.ckpt is None:
		print('No checkpoint provided')
		return

	ckpts = list(glob.glob(f'{args.ckpt}/*'))
	ckpt = sorted(ckpts)[-1] if len(ckpts) > 0 else None
	if ckpt is None:
		print('Checkpoint is invalid')
		return

	print('Restoring from %s' % ckpt)
	ckpt = torch.load(ckpt)
	model.load_state_dict(ckpt['model_state_dict'])

	img = Image.open(args.src)
	x = transform(img)
	x = x[None, :]
	model.eval()
	x = x.to(device)
	_, probs = model(x, args.quantize is not None, args.sparsify is not None)
	probs = torch.argmax(probs, 1)
	probs = probs.cpu().detach().numpy()
	print('Label: ', probs[0])

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--ckpt',
		type=str,
		help='Checkpoint')
	parser.add_argument(
		'src',
		type=str,
		help='Source image')
	parser.add_argument(
		'--sparsify',
		type=str,
		help='Sparsification scheme')
	parser.add_argument(
		'--quantize',
		type=str,
		help='Quantization scheme')
	args = parser.parse_args()
	main(args)
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

def get_accuracy(model, data_loader, device, quantize, sparsify):
	correct_pred = 0 
	n = 0

	with torch.no_grad():
		model.eval()
		for x, y_true in data_loader:
			x = x.to(device)
			y_true = y_true.to(device)

			_, y_prob = model(x, quantize, sparsify)
			_, predicted_labels = torch.max(y_prob, 1)

			n += y_true.size(0)
			correct_pred += (predicted_labels == y_true).sum()

	return correct_pred.float() / n

def main(args):
	device = 'cuda' if torch.cuda.is_available() else 'cpu'

	transform = transforms.Compose([
		transforms.Resize((HEIGHT, WIDTH)),
		transforms.ToTensor()])
	
	data, labels = args.test.split(',')

	dataset = HMB010Dataset(data, labels, 
		CLASSES, transform=transform)

	loader = DataLoader(dataset, 1, shuffle=True)

	quantizer = QUANTIZERS[args.quantize] if args.quantize else None
	sparsifier = SPARSIFIERS[args.sparsify] if args.sparsify else None

	model = LeNet(CLASSES, quantizer, sparsifier).to(device)

	ckpts = list(glob.glob(f'{args.ckpt}/*'))
	ckpt = sorted(ckpts)[-1] if len(ckpts) > 0 else None
	if ckpt is None:
		print('Checkpoint is invalid')
		return

	print('Restoring from %s' % ckpt)
	ckpt = torch.load(ckpt)
	model.load_state_dict(ckpt['model_state_dict'])

	model.eval()
	with torch.no_grad():
		acc = get_accuracy(model, loader, device,
			args.quantize is not None, args.sparsify is not None)
		print(f'Test accuracy: {100 * acc:.2f}')

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--ckpt',
		type=str,
		help='Checkpoint')
	parser.add_argument(
		'--test',
		type=str,
		help='Test data and labels')
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
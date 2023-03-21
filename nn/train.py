import torch
import argparse
import torch.nn as nn

from tqdm import tqdm
from datetime import datetime
from torch.utils.data import DataLoader
from torchvision import transforms

from globals import *
from lenet import *
from compress import QUANTIZERS_TORCH as QUANTIZERS
from compress import SPARSIFIERS_TORCH as SPARSIFIERS

def get_accuracy(model, data_loader, device):
	correct_pred = 0 
	n = 0

	with torch.no_grad():
		model.eval()
		for x, y_true in data_loader:
			x = x.to(device)
			y_true = y_true.to(device)

			_, y_prob = model(x)
			_, predicted_labels = torch.max(y_prob, 1)

			n += y_true.size(0)
			correct_pred += (predicted_labels == y_true).sum()

	return correct_pred.float() / n

def train(train_loader, model, criterion, optimizer, sparsify, quantize, device):
	model.train()
	running_loss = 0

	for x, y_true in train_loader:
		optimizer.zero_grad()

		x = x.to(device)
		y_true = y_true.to(device)

		# Forward pass
		y_hat, _ = model(x, sparsify, quantize) 
		loss = criterion(y_hat, y_true) 
		running_loss += loss.item() * x.size(0)

		# Backward pass
		loss.backward()
		optimizer.step()

	epoch_loss = running_loss / len(train_loader.dataset)
	return model, optimizer, epoch_loss

def validate(valid_loader, model, criterion, sparsify, quantize, device):
	model.eval()
	running_loss = 0

	for x, y_true in valid_loader:
		x = x.to(device)
		y_true = y_true.to(device)

		# Forward pass and record loss
		y_hat, _ = model(x, sparsify, quantize) 
		loss = criterion(y_hat, y_true) 
		running_loss += loss.item() * x.size(0)

	epoch_loss = running_loss / len(valid_loader.dataset)

	return model, epoch_loss

def main(args):
	''' 
	Main function, this is the entry point of training
	''' 
	device = 'cuda' if torch.cuda.is_available() else 'cpu'

	# combine the Resize() action and ToTensor() action togetherer
	transform = transforms.Compose([
		transforms.Resize((HEIGHT, WIDTH)),
		transforms.ToTensor()])

	# the [train] arg should be in the form "<train_data>,<train_labels>"
	train_data, train_labels = args.train.split(',')
	# the [val] arg should be in the form "<valid_data>,<valid_labels>"
	valid_data, valid_labels = args.val.split(',')

	train_dataset = HMB010Dataset(train_data, train_labels, 
		CLASSES, transform=transform)
	valid_dataset = HMB010Dataset(valid_data, valid_labels, 
		CLASSES, transform=transform)

	train_loader = DataLoader(train_dataset, args.batch, shuffle=True)
	valid_loader = DataLoader(valid_dataset, args.batch, shuffle=True)
	
	quantizer = QUANTIZERS[args.quantize] if args.quantize else None
	sparsifier = SPARSIFIERS[args.sparsify] if args.sparsify else None

	model = LeNet(CLASSES, quantizer, sparsifier).to(device)
	optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)#, weight_decay=1e-2)
	# optimizer = torch.optim.lr_scheduler.StepLR(optimizer, step_size=10, gamma=0.1)
	criterion = nn.CrossEntropyLoss()

	start_epoch = 0
	if args.ckpt and not args.overwrite:
		ckpts = list(glob.glob(f'{args.ckpt}/*'))
		ckpt = None
		for c in ckpts:
			m = re.findall('\d+', os.path.basename(c))
			epoch = int(m[0])
			if epoch >= start_epoch:
				ckpt = c
				start_epoch = epoch;

		if ckpt is not None:
			print('Restoring from %s' % ckpt)
			ckpt = torch.load(ckpt)
			model.load_state_dict(ckpt['model_state_dict'])
			optimizer.load_state_dict(ckpt['optimizer_state_dict'])

	quantize_debug = False
	sparsify_debug = False
	for epoch in tqdm(range(start_epoch, args.epochs)):
		quantize = args.quantize is not None and epoch >= args.optimize_epoch
		sparsify = args.sparsify is not None and epoch >= args.optimize_epoch

		if quantize and not quantize_debug:
			tqdm.write('[DEBUG] Quantization on')
			quantize_debug = True

		if sparsify and not sparsify_debug:
			tqdm.write('[DEBUG] Sparsification on')
			sparsify_debug = True

		# training
		model, optimizer, train_loss = \
			train(train_loader, model, criterion, 
				optimizer, quantize, sparsify, device)

		# validation
		with torch.no_grad():
			model, valid_loss = validate(valid_loader, model, criterion, 
				quantize, sparsify, device)

		train_acc = get_accuracy(model, train_loader, device=device)
		valid_acc = get_accuracy(model, valid_loader, device=device)

		nonzero = model.nonzero()

		tqdm.write(f'{datetime.now().time().replace(microsecond=0)} --- '
			f'Epoch: {epoch}\t'
			f'Train loss: {train_loss:.4f}\t'
			f'Valid loss: {valid_loss:.4f}\t'
			f'Train accuracy: {100 * train_acc:.2f}\t'
			f'Valid accuracy: {100 * valid_acc:.2f}\t'
			f'Nonzero: {nonzero}\t'
			f'Footprint: {(nonzero * 2) / 1024:.2f}KB')

		if args.ckpt:
			ckpt_path = os.path.join(args.ckpt, f'lenet{epoch:03d}.ckpt')
			torch.save({
				'model_state_dict': model.state_dict(),
				'optimizer_state_dict': optimizer.state_dict()
			}, ckpt_path)

	if args.onnx:
		dummy_input = torch.randn(1, 1, HEIGHT, WIDTH, device=device)
		input_names = ['img']
		output_names = ['label']
		torch.onnx.export(model, dummy_input, args.onnx,
			input_names=input_names, output_names=output_names)

if __name__ == '__main__':
	''' 
	Global entry point
	''' 
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--epochs',
		type=int,
		default=10,
		help='Epochs')
	parser.add_argument(
		'--lr',
		type=float,
		default=0.001,
		help='Learning rate')
	parser.add_argument(
		'--batch',
		type=int,
		default=16,
		help='Batch size')
	parser.add_argument(
		'--val',
		type=str,
		help='Validation data (img,lbls)')
	parser.add_argument(
		'--train',
		type=str,
		help='Train data (img,lbls)')
	parser.add_argument(
		'--ckpt',
		type=str,
		help='Checkpoint')
	parser.add_argument(
		'--onnx',
		type=str,
		help='Overwrite checkpoint')
	parser.add_argument(
		'--overwrite',
		action='store_true',
		help='Overwrite checkpoint')
	parser.add_argument(
		'--sparsify',
		type=str,
		help='Sparsification scheme')
	parser.add_argument(
		'--quantize',
		type=str,
		help='Quantization scheme')
	parser.add_argument(
		'--optimize-epoch',
		type=int,
		help='Epoch to sparsify/quantize')
	args = parser.parse_args()
	main(args)
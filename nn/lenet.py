import os
import re
import csv
import glob
import torch
import torch.nn as nn

from PIL import Image
from torch.utils.data import Dataset

from globals import *
from layers import *

class LeNet(nn.Module):
	def __init__(self, classes, quantizer=None, sparsifier=None):
		super(LeNet, self).__init__()

		k = 5
		downscale = 2
		width = WIDTH / downscale
		height = HEIGHT / downscale
		conv1_out_channels = 8
		conv2_out_channels = 16
		conv3_out_channels = 32
		fc1_out_features = 64

		v = k - 1
		w = int((int((width - v) / 2) - v) / 2) - v
		h = int((int((height - v) / 2) - v) / 2) - v
		in_features = int(w * h * conv3_out_channels)

		self.conv = QSequential(
			nn.MaxPool2d(kernel_size=downscale),         
			QConv2d(in_channels=1, out_channels=conv1_out_channels, 
				kernel_size=5, stride=1, padding=0, bias=False, 
				quantizer=quantizer, sparsifier=sparsifier),
			nn.ReLU(),
			nn.MaxPool2d(kernel_size=2),
			QConv2d(in_channels=conv1_out_channels, 
				out_channels=conv2_out_channels, 
				kernel_size=5, stride=1, padding=0, bias=False, 
				quantizer=quantizer, sparsifier=sparsifier),
			nn.ReLU(),
			nn.MaxPool2d(kernel_size=2),
			QConv2d(in_channels=conv2_out_channels, 
				out_channels=conv3_out_channels, 
				kernel_size=5, stride=1, padding=0, bias=False,  
				quantizer=quantizer, sparsifier=sparsifier),
			nn.ReLU()
		)

		self.fc = QSequential(
			QLinear(in_features=in_features, out_features=fc1_out_features, 
				quantizer=quantizer, sparsifier=sparsifier),
			nn.ReLU(),
			QLinear(in_features=64, out_features=classes, 
				quantizer=quantizer, sparsifier=sparsifier),
		)

		conv1_params = 1 * conv1_out_channels * 5 * 5
		conv2_params = conv1_out_channels * conv2_out_channels * 5 * 5
		conv3_params = conv2_out_channels * conv3_out_channels * 5 * 5
		fc1_params = in_features * fc1_out_features
		fc2_params = fc1_out_features * classes
		params = conv1_params + conv2_params + conv3_params
		params += fc1_params + fc2_params
		print('Params:', params, 'footprint:', (params * 2) / 1024, 'KB')
		
		conv1_activations = conv1_out_channels * (width - v) * (height - v)
		conv2_activations = conv2_out_channels * ((width - v) / 2 - v)
		conv2_activations *= ((height - v) / 2 - v)
		activations = max(conv1_activations, conv2_activations, in_features)
		print('Activations:', activations, 
			'footprint', (activations * 2) / 1024, 'KB', end='')
		if activations == conv1_activations:
			print('	=> Conv1')
		elif activations == conv2_activations:
			print('	=> Conv2')
		else:
			print('	=> Conv3')

	def forward(self, x, quantize=False, sparsify=False):
		x = self.conv(x, quantize, sparsify)
		x = torch.flatten(x, 1)
		logits = self.fc(x, quantize, sparsify)
		probs = nn.functional.softmax(logits, dim=1)
		return logits, probs

	def nonzero(self):
		return self.conv.nonzero() + self.fc.nonzero()

class HMB010Dataset(Dataset):
	def __init__(self, data_dir, label_file, classes, ext='png', transform=None):
		self.data = list(glob.glob(f'{data_dir}/*.{ext}'))
		self.classes = classes
		self.transform = transform
		self.labels = {}
		with open(label_file) as f:
			reader = csv.reader(f, delimiter=',')
			for (img, label) in reader:
				self.labels[img] = int(label)

	def __len__(self):
		return len(self.data)

	def __getitem__(self, idx):
		path = self.data[idx]
		file = os.path.basename(path)
		img = Image.open(path)
		label = self.labels[file]

		if self.transform is not None:
			img = self.transform(img)

		return img, label
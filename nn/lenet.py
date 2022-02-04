import os
import re
import csv
import sys
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

		p = 0.3
		k = 3
		downscale = 4
		width = WIDTH / downscale
		height = HEIGHT / downscale
		conv1_out_channels = 1
		conv2_out_channels = 8
		conv3_out_channels = 8
		fc1_out_features = 64

		v = k - 1
		cdim = lambda x: int((x - v) / 2)
		w = cdim(cdim(width))
		h = cdim(cdim(height))
		in_features = int(w * h * conv3_out_channels)

		self.conv = QSequential(
			nn.MaxPool2d(kernel_size=downscale),
			QConv2d(in_channels=conv1_out_channels,
				out_channels=conv1_out_channels,
				kernel_size=(k, 1), stride=1, padding=0, bias=False,
				quantizer=quantizer, sparsifier=sparsifier),
			QConv2d(in_channels=conv1_out_channels,
				out_channels=conv1_out_channels,
				kernel_size=(1, k), stride=1, padding=0, bias=False,
				quantizer=quantizer, sparsifier=sparsifier),
			QConv2d(in_channels=conv1_out_channels,
				out_channels=conv2_out_channels,
				kernel_size=(1, 1), stride=1, padding=0, bias=False,
				quantizer=quantizer, sparsifier=sparsifier),
			nn.ReLU(),
			nn.MaxPool2d(kernel_size=2),
			QConv2d(in_channels=conv2_out_channels,
				out_channels=conv2_out_channels,
				kernel_size=(k, 1), stride=1, padding=0, bias=False,
				quantizer=quantizer, sparsifier=sparsifier),
			QConv2d(in_channels=conv2_out_channels,
				out_channels=conv2_out_channels,
				kernel_size=(1, k), stride=1, padding=0, bias=False,
				quantizer=quantizer, sparsifier=sparsifier),
			QConv2d(in_channels=conv2_out_channels,
				out_channels=conv3_out_channels,
				kernel_size=(1, 1), stride=1, padding=0, bias=False,
				quantizer=quantizer, sparsifier=sparsifier),
			nn.ReLU(),
			nn.MaxPool2d(kernel_size=2),
		)

		self.fc = QSequential(
			QLinear(in_features=in_features, out_features=fc1_out_features, 
				quantizer=quantizer, sparsifier=sparsifier),
			nn.Dropout(p=p),
			nn.ReLU(),
			QLinear(in_features=fc1_out_features, out_features=classes, 
				quantizer=quantizer, sparsifier=sparsifier),
		)

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
		label = int(self.labels[file] > 0)

		if self.transform is not None:
			img = self.transform(img)

		return img, label
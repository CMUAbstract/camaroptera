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

'''
The class of LeNet model

:field conv: a method to create convolution layers
:field   fc: a method to create fully connected layers
'''
class LeNet(nn.Module):
	def __init__(self, classes, quantizer=None, sparsifier=None):
		'''
		The init method for LeNet Class

		:param    classes: size of each output sample 
		:param  quantizer: the quantizer to use
		:param sparsifier: the sparsifier to use
		'''
		super(LeNet, self).__init__()

		# set parameters
		p = 0.3 # probability of an element to be zeroed. Default: 0.5
		k = 3 # the second dimension of the size of the convolving kernel
		downscale = 4
		width = WIDTH / downscale # downscaled width
		height = HEIGHT / downscale # downscaled height
		conv1_out_channels = 1 # number of channels in the input image in conv1
		conv2_out_channels = 8 # number of channels in the input image in conv2
		conv3_out_channels = 8 # number of channels in the input image in conv3
		# the number of features in the flatten layer that connects the conv layers with the fc1 layer
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
	'''
	The class for storing metadata of the dataset

	:field:       data: a list of names of all image files
	:field:    classes: size of each output sample
	:field: transforms: the combined transforms to process images
	:field:     labels: a dict mapping each filename to its label
	'''
	def __init__(self, data_dir, label_file, classes, ext='png', transform=None):
		'''
		The init method for HMB01Dataset Class

		:param   data_dir: the directory for the photos
		:param label_file: the name of the label file
		:param    classes: size of each output sample
		:param        ext: the file extension for input files
		:param  transform: the combined transforms
		'''
		self.data = list(glob.glob(f'{data_dir}/*.{ext}'))
		self.classes = classes
		self.transform = transform
		self.labels = {}
		with open(label_file) as f:
			reader = csv.reader(f, delimiter=',')
			for (img, label) in reader:
				self.labels[img] = int(label)

	def __len__(self):
		'''
		Return the length of the image files list(self.data)
		'''
		return len(self.data)

	def __getitem__(self, idx):
		'''
		Return the idx_th image file name and the label for this image file

		:param    idx: the index of the wanted data
		:return   img: the opened and transformed file. Most likely to be a tensor
		:return label: the label for the image. The label will be a boolean value
		'''
		path = self.data[idx]
		file = os.path.basename(path)
		img = Image.open(path)
		label = int(self.labels[file] > 0)

		if self.transform is not None:
			img = self.transform(img)

		return img, label
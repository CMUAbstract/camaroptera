import torch
import torch.nn as nn

class QConv2d(nn.Conv2d):
	def __init__(self, in_channels, out_channels, kernel_size, 
		stride=1, padding=0, dilation=1, groups=1, bias=True, 
		padding_mode='zeros', device=None, dtype=None, 
		quantizer=None, sparsifier=None):
		super(QConv2d, self).__init__(in_channels, out_channels, 
			kernel_size, stride, padding, dilation, groups, 
			bias, padding_mode, device, dtype)
		identity = lambda x : x
		self.sparsifier = sparsifier if sparsifier else identity
		self.quantizer = quantizer if quantizer else identity

	def quantize(self, quantize, sparsify):
		weight = self.weight
		bias = self.bias

		if quantize:
			weight = self.quantizer(weight)
			if self.bias is not None:
				bias = self.quantizer(bias)

		if sparsify:
			weight = self.sparsifier(weight)
			if self.bias is not None:
				bias = self.sparsifier(bias)

		return weight, bias

	def forward(self, input, quantize=False, sparsify=False):
		weight, bias = self.quantize(quantize, sparsify)
		return nn.functional.conv2d(input, weight, bias, 
			self.stride, self.padding, self.dilation, self.groups)

	def nonzero(self):
		weight, _ = self.quantize(True, True)
		return torch.count_nonzero(weight)

class QLinear(nn.Linear):
	def __init__(self, in_features, out_features, bias=True,
		device=None, dtype=None, quantizer=None, sparsifier=None):
		super(QLinear, self).__init__(in_features, out_features, bias,
			device, dtype)
		identity = lambda x : x
		self.sparsifier = sparsifier if sparsifier else identity
		self.quantizer = quantizer if quantizer else identity

	def quantize(self, quantize, sparsify):
		weight = self.weight
		bias = self.bias

		if quantize:
			weight = self.quantizer(weight)
			if self.bias is not None:
				bias = self.quantizer(bias)

		if sparsify:
			weight = self.sparsifier(weight)
			if self.bias is not None:
				bias = self.sparsifier(bias)

		return weight, bias

	def forward(self, input, quantize=False, sparsify=False):
		weight, bias = self.quantize(quantize, sparsify)
		return nn.functional.linear(input, weight, bias)

	def nonzero(self):
		weight, _ = self.quantize(True, True)
		return torch.count_nonzero(weight)

class QSequential(nn.Sequential):
	def __init__(self, *args):
		super(QSequential, self).__init__(*args)

	def forward(self, input, quantize=False, sparsify=False):
		layers = [QSequential, QLinear, QConv2d]
		for module in self:
			if any([isinstance(module, x) for x in layers]):
				input = module(input, quantize, sparsify)
			else:
				input = module(input)

		return input

	def nonzero(self):
		count = 0
		for module in self:
			if hasattr(module, 'nonzero'):
				count += module.nonzero()
		return count
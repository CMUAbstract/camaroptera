import torch
import torch.nn as nn

class PrintLayer(nn.Module):
    def __init__(self):
        super(PrintLayer, self).__init__()
    
    def forward(self, x):
        print(x)
        return x

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

	def forward(self, input, quantize=False, sparsify=False):
		quantizer = self.quantizer if quantize else lambda x : x
		sparsifier = self.sparsifier if sparsify else lambda x : x
		
		weight = quantizer(sparsifier(self.weight))
		bias = None if self.bias is None else quantizer(sparsifier(self.bias))

		act = nn.functional.conv2d(input, weight, bias, 
			self.stride, self.padding, self.dilation, self.groups)
		return quantizer(act)

	def nonzero(self):
		quantizer = self.quantizer
		sparsifier = self.sparsifier
		weight = quantizer(sparsifier(self.weight))
		return torch.count_nonzero(weight)

class QLinear(nn.Linear):
	def __init__(self, in_features, out_features, bias=True,
		device=None, dtype=None, quantizer=None, sparsifier=None):
		super(QLinear, self).__init__(in_features, out_features, bias,
			device, dtype)
		identity = lambda x : x
		self.sparsifier = sparsifier if sparsifier else identity
		self.quantizer = quantizer if quantizer else identity

	def forward(self, input, quantize=False, sparsify=False):
		quantizer = self.quantizer if quantize else lambda x : x
		sparsifier = self.sparsifier if sparsify else lambda x : x
		
		weight = quantizer(sparsifier(self.weight))
		bias = None if self.bias is None else quantizer(sparsifier(self.bias))

		act = nn.functional.linear(input, weight, bias)
		return quantizer(act)

	def nonzero(self):
		quantizer = self.quantizer
		sparsifier = self.sparsifier
		weight = quantizer(sparsifier(self.weight))
		return torch.count_nonzero(weight)

# These batchnorm layers aren't quite right given division
class QBatchNorm2d(nn.BatchNorm2d):
	def __init__(self, num_features, eps=1e-05, momentum=0.1, affine=True, 
		track_running_stats=True, device=None, dtype=None, 
		quantizer=None, sparsifier=None):
		super(QBatchNorm2d, self).__init__(num_features, 
			eps, momentum, affine, track_running_stats, device, dtype)
		identity = lambda x : x
		self.sparsifier = sparsifier if sparsifier else identity
		self.quantizer = quantizer if quantizer else identity

	def forward(self, input, quantize=False, sparsify=False):
		quantizer = self.quantizer if quantize else lambda x : x
		sparsifier = self.sparsifier if sparsify else lambda x : x

		running_mean = quantizer(sparsifier(self.running_mean))
		running_var = quantizer(sparsifier(self.running_var))
		
		act = torch.nn.functional.batch_norm(input, running_mean, running_var, 
			self.weight, self.bias, self.training, self.momentum, self.eps)
		return quantizer(act)

	def nonzero(self):
		return 2

class QBatchNorm1d(nn.BatchNorm1d):
	def __init__(self, num_features, eps=1e-05, momentum=0.1, affine=True, 
		track_running_stats=True, device=None, dtype=None,
		quantizer=None, sparsifier=None):
		super(QBatchNorm1d, self).__init__(num_features, 
			eps, momentum, affine, track_running_stats, device, dtype)
		identity = lambda x : x
		self.sparsifier = sparsifier if sparsifier else identity
		self.quantizer = quantizer if quantizer else identity

	def forward(self, input, quantize=False, sparsify=False):
		quantizer = self.quantizer if quantize else lambda x : x
		sparsifier = self.sparsifier if sparsify else lambda x : x

		running_mean = quantizer(sparsifier(self.running_mean))
		denom = torch.sqrt(self.running_var + self.eps)
		denom = quantizer(sparsifier(denom))
		
		running_mean = quantizer(sparsifier(self.running_mean))
		running_var = quantizer(sparsifier(self.running_var))
		
		act = torch.nn.functional.batch_norm(input, running_mean, running_var, 
			self.weight, self.bias, self.training, self.momentum, self.eps)
		return quantizer(act)

	def nonzero(self):
		return 2

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

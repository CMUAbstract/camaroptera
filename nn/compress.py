'''
This file contains some self-defined quantizers,
sparcifiers, and some global variabls
'''
import torch
import numpy as np

def int_quantizer_torch(bits, scale=128):
	dtype_max = 2 ** (bits - 1)

	clip_ = lambda x: torch.relu(x + dtype_max) - dtype_max
	clip = lambda x: -clip_(-clip_(x))

	class quantizer(torch.autograd.Function):
		@staticmethod
		def forward(ctx, x, debug=''):
			rounded = torch.round(scale * x)
			clipped = clip(rounded)

			return clip(rounded) / scale
		
		@staticmethod
		def backward(ctx, grad_output):
			return grad_output

	return quantizer.apply

def int_quantizer_np(bits, scale=128):
	dtype_max = 2 ** (bits - 1)
	def quantizer(x):
		rounded = (scale * x).round()
		return np.clip(rounded, -dtype_max, dtype_max - 1)

	return quantizer

def threshold_sparsifier_torch(threshold):
	class sparsifier(torch.autograd.Function):
		@staticmethod
		def forward(ctx, x):
			x = x.clone()
			x[torch.abs(x) < threshold] = 0
			return x
		
		@staticmethod
		def backward(ctx, grad_output):
			return grad_output

	return sparsifier.apply

def threshold_sparsifier_np(threshold):
	def sparsifier(x):
		x[np.abs(x) < threshold] = 0
		return x
	return sparsifier

QUANTIZERS_TORCH = {
	'int16' : int_quantizer_torch(16, 2**8),
	'int8' : int_quantizer_torch(8, 2**4),
}

QUANTIZERS_NP = {
	'int16' : int_quantizer_np(16, 2**8),
	'int8' : int_quantizer_np(8, 2**4)
}

SPARSIFIERS_TORCH = {
	'threshold': threshold_sparsifier_torch(0.031)
}

SPARSIFIERS_NP = {
	'threshold': threshold_sparsifier_np(0.031)
}
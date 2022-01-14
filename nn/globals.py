import torch

WIDTH = 160
HEIGHT = 120
CLASSES = 2

def int_quantizer(bits):
	scale = 2 ** (bits - 1)
	def quantizer(x):
		rounded = torch.round(scale * x)
		return torch.clip(rounded, min=-scale, max=scale - 1) / scale
	return quantizer

QUANTIZERS = {
	'int16' : int_quantizer(16),
	'int8' : int_quantizer(8)
}

def threshold_sparsifier(threshold):
	def sparsifier(x):
		x = x.clone()
		x[torch.abs(x) < threshold] = 0
		return x
	return sparsifier

SPARSIFIERS = {
	'threshold': threshold_sparsifier(0.02)
}
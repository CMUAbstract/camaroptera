import os
import onnx
import argparse
import numpy as np

from scipy.sparse import csr_matrix
from onnx import numpy_helper

def int_quantizer(bits, normalize=True):
	scale = 2 ** (bits - 1)
	def quantizer(x):
		rounded = (scale * x).round()
		return np.clip(rounded, -scale, scale - 1)

	return quantizer

QUANTIZERS = {
	'int16' : int_quantizer(16),
	'int8' : int_quantizer(8),
}

def threshold_sparsifier(threshold):
	def sparsifier(x):
		x[np.abs(x) < threshold] = 0
		return x
	return sparsifier

SPARSIFIERS = {
	'threshold': threshold_sparsifier(0.02)
}

def is_int_dtype(dtype):
	return dtype in ['int16_t', 'int8_t', 'short', 'byte', 'uint16_t']

def dump_mat(f, name, w, dtype='int16_t'):
	dims = len(w.shape)
	f.write(f'__ro__hifram {dtype} {name}')

	for d in w.shape:
		f.write(f'[{d}]')

	f.write(' = {')

	for i, v in enumerate(w.flatten()):
		last = i == w.size - 1 
		if is_int_dtype(dtype):
			f.write(f'{int(v)}')
		else:
			f.write(f'F_LIT(int(v))')

		if not last:
			f.write(', ')

	f.write('};\n')

def to_header(f, name, w, sparse=False, dtype='int16_t'):
	f.write(f'#ifndef {name.upper()}\n')
	f.write(f'#define {name.upper()}\n')
	f.write('#include <libdnn/mem.h>\n')

	dims = len(w.shape)
	if dims == 4 and sparse:
		print(f'[WARNING] Dims==4 and sparse not implemented for {name}')
	elif dims == 4:
		dump_mat(f, name, w, dtype)
	elif dims == 2 and sparse: # HERE
		csr = csr_matrix(w)
		f.write(f'#define {name.upper()}_SIZE {csr.indptr.size}\n')
		dump_mat(f, f'{name}_indptr', csr.indptr, 'uint16_t')
		dump_mat(f, f'{name}_data', csr.data, dtype)
		dump_mat(f, f'{name}_index', csr.indices, 'uint16_t')
	elif dims == 2:
		dump_mat(f, name, w, dtype)
	elif dims == 1:
		dump_mat(f, name, w, dtype)
	else:
		print(f'[WARNING] Can\'t dump {name}')

	f.write('#endif\n')

def main(args):
	if args.dest is None:
		print('Destination is none')
		return

	model = onnx.load(args.src)
	weights = {}

	quantizer = QUANTIZERS[args.quantize] if args.quantize else lambda x: x
	sparsifier = SPARSIFIERS[args.sparsify] if args.sparsify else lambda x: x

	for weight in model.graph.initializer:
		name = weight.name
		weight = numpy_helper.to_array(weight).copy()
		weight = sparsifier(weight)
		weight = quantizer(weight)

		weights[name] = weight

	for name in weights:
		weight = weights[name]
		name = name.replace('.', '_')
		path = os.path.join(args.dest, f'{name}.h')

		sparse = True
		density = np.count_nonzero(weight) / weight.size
		if 'bias' in name:
			sparse = False
		elif density > args.density_threshold:
			sparse = False

		with open(path, 'w+') as f:
			print(f'[DEBUG] Dumping {name} Sparse? {sparse} (Density: {density})')
			to_header(f, name, weight, sparse, args.dtype)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--dest',
		type=str,
		help='Header destination')
	parser.add_argument(
		'src',
		type=str,
		help='Onnx')
	parser.add_argument(
		'--density-threshold',
		type=float,
		default=0.5,
		help='Density threshold')
	parser.add_argument(
		'--dtype',
		type=str,
		default='int16_t',
		help='Density threshold')
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
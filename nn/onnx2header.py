import os
import onnx
import argparse
import numpy as np

from scipy.sparse import csr_matrix
from onnx import numpy_helper
from onnx import shape_inference

from compress import QUANTIZERS_NP as QUANTIZERS
from compress import SPARSIFIERS_NP as SPARSIFIERS 

# TAB = '  '
TAB = '\t'
SUPPORTED_OPS = {
	'Conv' : 'conv',
	'Gemm' : 'fc',
	'MaxPool' : 'pool',
	'Relu' : 'relu'
}

DTYPE_2_SHIFT = {
	'int16' : 8,
	'int8' : 4
}

DTYPE_2_CTYPE = {
	'int16' : 'int16_t',
	'int8' : 'int8_t',
	'uint16' : 'uint16_t',
	'uint8' : 'uint8_t',
}

def dump_mat(f, name, w, dtype='int16'):
	dims = len(w.shape)
	f.write(f'__ro_hifram {DTYPE_2_CTYPE[dtype]} {name}')

	for d in w.shape:
		f.write(f'[{d}]')

	f.write(' = {')

	for i, v in enumerate(w.flatten()):
		last = i == w.size - 1 
		f.write(f'{int(v)}')

		if not last:
			f.write(', ')

	f.write('};\n')

	print(f'[DEBUG] {name} is {w.size / 512}KB w/ shape {w.shape}')
	return w.size

def to_header(f, name, w, sparse=False, dtype='int16_t'):
	f.write(f'#ifndef {name.upper()}\n')
	f.write(f'#define {name.upper()}\n')
	f.write('#include "cam_mlkernels.h"\n')

	sz = 0
	dims = len(w.shape)
	if dims == 4 and not sparse:
		sz += dump_mat(f, name, w, dtype)
	elif dims == 2 and sparse: # HERE
		csr = csr_matrix(w)
		f.write(f'#define {name.upper()}_SIZE {csr.data.size}\n')
		sz += dump_mat(f, f'{name}_indptr', csr.indptr, 'uint16')
		f.write('\n');
		sz += dump_mat(f, f'{name}_index', csr.indices, 'uint16')
		f.write('\n');
		sz += dump_mat(f, f'{name}', csr.data, dtype)
	elif dims == 2:
		sz += dump_mat(f, name, w, dtype)
	elif dims == 1:
		sz += dump_mat(f, name, w, dtype)
	else:
		print(f'[WARNING] Can\'t dump {name}')

	f.write('#endif\n')
	return sz

def dump_include(path, name):
	return f'#include "{path}/{name}.h"\n'

def dump_mat_struct(name, w, sparse, dtype='int16_t'):
	dims = len(w.shape)
	s = f'__ro_fram mat_t mat_{name} = {{\n';
	if sparse:
		s += f'{TAB}.dims = {{{name.upper()}_SIZE}},\n'
		s += f'{TAB}.strides = {1},\n'
		s += f'{TAB}.len_dims = 1,\n'
		s += f'{TAB}.data = {name},\n'
		s += f'{TAB}.sparse = {{\n'
		s += f'{TAB}{TAB}.dims = {{'

		if dims == 4:
			s += f'{w.shape[0]}, {w.shape[1]}, {w.shape[2]}, {w.shape[3]}'
		elif dims == 2:
			s += f'{w.shape[0]}, {w.shape[1]}'
		else:
			s += f'{w.shape[0]}, 1'

		s += f'}},\n'

		s += f'{TAB}{TAB}.len_dims = {dims},\n'
		s += f'{TAB}{TAB}.sizes = {name}_indptr,\n'
		s += f'{TAB}{TAB}.offsets = {name}_index\n'
		s += f'{TAB}}}'
	else:
		s += f'{TAB}.dims = {{'

		if dims == 4:
			s += f'{w.shape[0]}, {w.shape[1]}, {w.shape[2]}, {w.shape[3]}'
		elif dims == 2:
			s += f'{w.shape[0]}, {w.shape[1]}'
		else:
			s += f'{w.shape[0]}, 1'

		s += f'}},\n'

		s += f'{TAB}.strides = {{'
		if dims == 4:
			s += f'{w.shape[1] * w.shape[2] * w.shape[3]}, '
			s += f'{w.shape[2] * w.shape[3]}, {w.shape[3]}, 1'
		elif dims == 2:
			s += f'{w.shape[1]}, 1'
		else:
			s += f'1, 1'

		s += f'}},\n'
		
		s += f'{TAB}.len_dims = {dims},\n'
		s += f'{TAB}.data = {name}\n'
	s += f'}};'
	return s

def dump_task(node, shape, sparse, idx, src, dest, ttype='int16'):
	def dump_shape():
		s = ''
		if len(shape) == 1:
			shape.append(1)

		for i, d in enumerate(shape):
			s += f'{d}'
			if i < len(shape) - 1:
				s += ', '
		return s

	def get_attr(attr):
		for attribute in node.attribute:
			if attribute.name == attr:
				return attribute.ints
		return None

	def get_ptr_with_sub(sub):
		for input in node.input:
			if sub in input:
				return '&mat_' + input.replace('.', '_')
		return 'NULL'

	shift = DTYPE_2_SHIFT[ttype]
	s = f'void dnn_L{idx}_'

	unsupported = node.op_type not in SUPPORTED_OPS
	if unsupported:
		print(f'Not supported {node.name} with op_type {node.op_type}')
	else:
		s += f'{SUPPORTED_OPS[node.op_type]}() {{\n'

	if 'MaxPool' in node.op_type:
		s += f'{TAB}MAT_RESHAPE({dest}, {dump_shape()});\n'
		kernel_size = get_attr('kernel_shape')[0]
		stride = get_attr('strides')[0]
		s += f'{TAB}pooling({src}, {dest}, 0, {kernel_size}, {stride});\n'
	elif 'Relu' in node.op_type:
		s += f'{TAB}MAT_RESHAPE({dest}, {dump_shape()});\n'
		s += f'{TAB}relu({dest}, 0);\n'
	elif 'Gemm' in node.op_type:
		s += f'{TAB}// MAT_RESHAPE({src}, FILL, 1);\n'
		s += f'{TAB}MAT_RESHAPE({dest}, {dump_shape()});\n'

		weight = get_ptr_with_sub('weight')
		bias = get_ptr_with_sub('bias')
		s += f'{TAB}mat_t *w_ptr = {weight};\n'
		s += f'{TAB}mat_t *b_ptr = {bias};\n'

		if sparse:
			s += f'{TAB}fc_sparse(w_ptr, b_ptr, {src}, {dest}, {shift});\n'
		else:
			s += f'{TAB}fc_dense(w_ptr, b_ptr, {src}, {dest}, {shift});\n'
	elif 'Conv' in node.op_type and not sparse:
		s += f'{TAB}MAT_RESHAPE({dest}, {dump_shape()});\n'
		
		weight = get_ptr_with_sub('weight')
		bias = get_ptr_with_sub('bias')
		s += f'{TAB}mat_t *w_ptr = {weight};\n'
		s += f'{TAB}mat_t *b_ptr = {bias};\n'

		stride = get_attr('strides')[0]
		s += f'{TAB}conv_dense(w_ptr, b_ptr, {src}, {dest}, {stride}, {shift});\n'

	s += '}\n'
	s = s if not unsupported else ''
	return s

def dump_def(node, idx):
	return f'void dnn_L{idx}_{SUPPORTED_OPS[node.op_type]}();\n'

def dump_call(node, idx):
	return f'dnn_L{idx}_{SUPPORTED_OPS[node.op_type]}();\n'

def main(args):
	if args.dest is None:
		print('Destination is none')
		return

	model = onnx.load(args.src)
	weights = {}
	shapes = {}

	quantizer = QUANTIZERS[args.quantize] if args.quantize else lambda x: x
	sparsifier = SPARSIFIERS[args.sparsify] if args.sparsify else lambda x: x

	for weight in model.graph.initializer:
		name = weight.name
		weight = numpy_helper.to_array(weight).copy()
		weight = sparsifier(weight)
		weight = quantizer(weight)

		weights[name] = weight

	inferred = shape_inference.infer_shapes(model)
	for value in inferred.graph.value_info:
		dims = []
		for d in value.type.tensor_type.shape.dim:
			dims.append(d.dim_value)
		shapes[value.name] = dims[1:]

	for value in inferred.graph.output:
		dims = []
		for d in value.type.tensor_type.shape.dim:
			dims.append(d.dim_value)
		shapes[value.name] = dims[1:]

	sz = 0
	mats = ''
	defs = ''
	includes = ''
	tasks = ''
	machine = {}
	control = ''
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
			sz += to_header(f, name, weight, sparse, args.dtype)
			mats += dump_mat_struct(name, weight, sparse, args.dtype) + '\n\n'
			includes += dump_include(args.dest, name)

	print(f'[DEBUG] Wrote {sz / 512}KB')

	src = 'b1'
	dest = 'b2'
	for i, node in enumerate(model.graph.node):
		sparse = False
		for input in node.input:
			if input in weights:
				weight = weights[input]
				density = np.count_nonzero(weight) / weight.size
				if 'bias' not in input and density <= args.density_threshold:
					sparse = True

		if node.output[0] not in shapes:
			print(f'[WARNING] {node.output[0]} not in shapes; not dumping {node.name}')
			continue

		if 'Relu' in node.op_type:
			src, dest = dest, src

		task = dump_task(node, shapes[node.output[0]], sparse, i, src, dest, args.ttype)
		if len(task) > 0:
			defs += dump_def(node, i)
			tasks += task + '\n'
			machine[i] = dump_call(node, i)

			if 'Relu' not in node.op_type:
				src, dest = dest, src
			else:
				src, dest = dest, src

	keys = sorted(machine.keys())
	for i, idx in enumerate(keys):
		control += 'else if' if i > 0 else 'if'
		control += f'(dnn_layer == {idx}) {{\n'
		control += f'{TAB}{machine[idx]}'
		if i < len(machine) - 1:
			next_idx = keys[i + 1]
			control += f'{TAB}UPDATE_STATE({next_idx});\n'
			control += f'{TAB}goto TOP;\n'
		else:
			control += f'{TAB}UPDATE_STATE({keys[0]});\n'
		control += '} '

	if args.generate_code:
		with open(args.generate_code, 'w+') as f:
			f.write(includes)
			f.write('\n')
			f.write(mats)
			f.write(tasks)
			f.write(defs)
			f.write('\n')
			f.write(control)

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
		default='int16',
		help='Deployed network datatype')
	parser.add_argument(
		'--ttype',
		type=str,
		default='int16',
		help='Trained network datatype')
	parser.add_argument(
		'--sparsify',
		type=str,
		help='Sparsification scheme')
	parser.add_argument(
		'--quantize',
		type=str,
		help='Quantization scheme')
	parser.add_argument(
		'--generate-code',
		type=str,
		help='Generate code')
	args = parser.parse_args()
	main(args)
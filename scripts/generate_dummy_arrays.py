import numpy as np
import argparse
import os

# Size of the trial to run
TRIAL_SIZE = 100

# Ratio of True Positives:True Negatives where they total to 100%
TP_RATE = 50
TN_RATE = 50

# Rates of False Positves and False Negatives
FP_RATE = 10
FN_RATE = 30

def main(args):

	true_positives = np.ones(TP_RATE*TRIAL_SIZE/100).astype(dtype="int")
	true_negatives = np.zeros(TN_RATE*TRIAL_SIZE/100).astype(dtype="int")
	input_images = np.append(true_positives, true_negatives)
	np.random.shuffle(input_images)

	fp_ones = np.ones(FP_RATE*len(true_negatives)/100).astype(dtype="int")
	fp_zeros = np.zeros( len(true_negatives) - len(fp_ones) ).astype(dtype="int")	
	false_positives = np.append(fp_zeros, fp_ones)
	np.random.shuffle(false_positives)

	fn_ones = np.ones(FN_RATE*len(true_positives)/100).astype(dtype="int")
	fn_zeros = np.zeros( len(true_positives) - len(fn_ones) ).astype(dtype="int")
	false_negatives = np.append(fn_zeros, fn_ones)
	np.random.shuffle(false_negatives)

	print(input_images)
	print(false_positives)
	print(false_negatives)

	path = os.path.join( args.output_dir, args.output_name ) 
	fp = open(path, 'w')

	file_data = '#include <stdbool.h>\n\n'
	file_data += '#include "lenet.h"\n\n'
 
	file_data += '__ro_hifram uint8_t image_sequence[' + str(len(input_images)) + '] = {'

	for i in range(len(input_images)):
		file_data += str(input_images[i])
		if i != len(input_images) - 1:
			file_data += ', '

	file_data += '};\n\n'

	file_data += '__ro_hifram uint8_t false_positives[' + str(len(false_positives)) + '] = {'

	for i in range(len(false_positives)):
		file_data += str(false_positives[i])
		if i != len(false_positives) - 1:
			file_data += ', '

	file_data += '};\n\n'

	file_data += '__ro_hifram uint8_t false_negatives[' + str(len(false_negatives)) + '] = {'

	for i in range(len(false_negatives)):
		file_data += str(false_negatives[i])
		if i != len(false_negatives) - 1:
			file_data += ', '

	file_data += '};\n'
	fp.write(file_data)
	fp.close

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--output_dir',
		type=str,
		help='Output directory')
	parser.add_argument(
		'--output_name',
		type=str,
		default="experiment_array.h",
		help='Name of output file')
	args = parser.parse_args()
	main(args)


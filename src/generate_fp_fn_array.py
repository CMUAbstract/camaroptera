import numpy as np
import argparse
import os

def generate_header(args, trial_size, tp_rate, tn_rate, fp_rate, fn_rate, num_trial):

	true_positives = np.ones(int(tp_rate*trial_size/100)).astype(dtype="int")
	true_negatives = np.zeros(int(tn_rate*trial_size/100)).astype(dtype="int")
	input_images = np.append(true_positives, true_negatives)
	np.random.shuffle(input_images)

	fp_ones = np.ones(int(np.ceil(fp_rate*len(true_negatives)/100))).astype(dtype="int")
	fp_zeros = np.zeros( len(true_negatives) - len(fp_ones) ).astype(dtype="int")	
	false_positives = np.append(fp_zeros, fp_ones)
	np.random.shuffle(false_positives)

	fn_ones = np.ones(int(np.ceil(fn_rate*len(true_positives)/100))).astype(dtype="int")
	fn_zeros = np.zeros( len(true_positives) - len(fn_ones) ).astype(dtype="int")
	false_negatives = np.append(fn_zeros, fn_ones)
	np.random.shuffle(false_negatives)

	print(input_images)
	print(false_positives)
	print(false_negatives)

	filename = "experiment_array_%s_%s_%s_%s_%s.h" % (tp_rate, tn_rate, fp_rate, fn_rate, num_trial)
	print(filename)
	
	path = os.path.join( args.output_dir, filename ) 
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

def main(args):
	# Size of the trial to run
	#trial_size = 100
	trial_size = args.trial_size
	num_trials = args.num_trials

	# Ratio of True Positives:True Negatives where they total to 100%
#	tp_rates = [ 20, 50, 80 ]
#	tn_rates = [ 80, 50, 20 ]
	tp_rates = [ 1 ]
	tn_rates = [ 99 ]

	# Rates of False Positves and False Negatives
#	fp_rates = [ 40, 20, 10 ]
#	fn_rates = [ 1, 5, 10 ]
	fp_rates = [ 10 ]
	fn_rates = [ 10 ]


	for exp_index in range(len(tp_rates)):
		for dnn_index in range(len(fp_rates)):
			for num_trial in range(num_trials):
				generate_header(args, trial_size, tp_rates[exp_index], tn_rates[exp_index], fp_rates[dnn_index], fn_rates[dnn_index], num_trial)

if __name__ == '__main__':
	parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
	parser.add_argument(
		'--output_dir',
		type=str,
		default='src/event_headers_for_experiments',
		help='Output directory')
	parser.add_argument(
		'--trial_size',
		type=int,
		default=100,
		help='Trial Size')
	parser.add_argument(
		'--num_trials',
		type=int,
		default=1,
		help='Number of trials')
	args = parser.parse_args()
	main(args)


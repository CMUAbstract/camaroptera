import numpy as np
import argparse
import os
import matplotlib.pyplot as plt

# Ratio of True Positives:True Negatives where they total to 100%
TP_RATE = 20
TN_RATE = 80

# Rates of False Positves and False Negatives
FP_RATE = 30
FN_RATE = 10

def main(args):
	
	trial_size = args.tnum
	
	high_times = np.random.normal( loc=3, scale=1, size=trial_size)
	for i in range(len(high_times)):
		high_times[i] = abs(high_times[i])
	print(high_times)
	
	count, bins, ignored = plt.hist(high_times)
	plt.show()

	low_times = np.random.poisson( lam=10, size=trial_size )
	print(low_times)

	count, bins, ignored = plt.hist(low_times)
	plt.show()
	
	true_positives = np.ones(TP_RATE*TRIAL_SIZE/100).astype(dtype="int")
	true_negatives = np.zeros(TN_RATE*TRIAL_SIZE/100).astype(dtype="int")

	fp_ones = np.ones(FP_RATE*len(true_negatives)/100).astype(dtype="int")
	fp_zeros = np.zeros( len(true_negatives) - len(fp_ones) ).astype(dtype="int")	
	false_positives = np.append(fp_zeros, fp_ones)
	np.random.shuffle(false_positives)

	fn_ones = np.ones(FN_RATE*len(true_positives)/100).astype(dtype="int")
	fn_zeros = np.zeros( len(true_positives) - len(fn_ones) ).astype(dtype="int")
	false_negatives = np.append(fn_zeros, fn_ones)
	np.random.shuffle(false_negatives)

	print(false_positives)
	print(false_negatives)

	path = os.path.join( args.output_dir, args.output_name ) 
	fp = open(path, 'w')

	file_data = '#include <stdbool.h>\n\n'
 
	file_data += 'float high_times[' + str(len(high_times)) + '] = {'

	for i in range(len(high_times)):
		file_data += str(high_times[i])
		if i != len(high_times) - 1:
			file_data += ', '

	file_data += '};\n\n'
	
	file_data += 'uint8_t low_times[' + str(len(low_times)) + '] = {'

	for i in range(len(low_times)):
		file_data += str(low_times[i])
		if i != len(low_times) - 1:
			file_data += ', '

	file_data += '};\n\n'

	file_data += 'uint8_t false_positives[' + str(len(false_positives)) + '] = {'

	for i in range(len(false_positives)):
		file_data += str(false_positives[i])
		if i != len(false_positives) - 1:
			file_data += ', '

	file_data += '};\n\n'

	file_data += 'uint8_t false_negatives[' + str(len(false_negatives)) + '] = {'

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
	parser.add_argument(
		'--tnum',
		type=int,
		default=10,
		help='Number of events to generate')
	parser.add_argument(
		'--tp',
		type=int,
		default=,
		help='True Positive Rate')
	args = parser.parse_args()
	main(args)


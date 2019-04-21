import scipy
import numpy as np

F_N = 5
F_ONE = 1 << F_N
to_fixed = lambda x: int(x * F_ONE)
to_float = lambda x: float(x) / F_ONE
to_frac = lambda x: x.as_integer_ratio()

def arctan2(x, buckets=9):
	bucket = 180 / 9
	y, x = to_frac(x)
	return (int(np.rad2deg(np.arctan2(float(y), float(x)))) % 180) / bucket

def main():
	print [arctan2(to_float(i)) for i in xrange(to_fixed(1))]
	print [arctan2(-to_float(i)) for i in xrange(to_fixed(1))]
	print [arctan2(1. / to_float(i)) if i != 0 else 4 for i in xrange(to_fixed(1))]
	print [arctan2(-1. / to_float(i)) if i != 0 else 4 for i in xrange(to_fixed(1))]

if __name__ == '__main__':
	main()
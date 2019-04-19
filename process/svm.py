
import numpy as np # linear algebra
import json
from matplotlib import pyplot as plt
from skimage import color, data, exposure
from skimage.feature import hog
from sklearn.svm import SVC
from sklearn.linear_model import SGDClassifier
from sklearn.metrics import classification_report,accuracy_score, confusion_matrix
from sklearn.externals import joblib

from skimage.io import imread_collection

import argparse
import time
import glob
import os
import sys
import cv2
import pickle

f_lit = lambda x: 'F_LIT(' + str(x) + ')'

def write_header(name, mats, output_file):
    contents = '#ifndef ' + name.upper() + '_H\n'
    contents += '#define ' + name.upper() + '_H\n'
    contents += '#include \'<libfixed/fixed.h>\'\n\n'
    for mat_name, mat, layer, sparse in mats:
            if layer == 'CONV' and sparse:
                    mat_str = ''
                    offsets_str = ''
                    sizes_str = ''
                    size = 0
                    mat = mat.reshape(mat.shape[0], -1)
                    for m in mat:
                            data = m[m != 0.0].astype(dtype=str)

                            idx = np.where(m != 0.0)[0]
                            offsets = np.diff(idx).flatten()
                            if data.shape[0] > 0:
                                    data_size = data.flatten().shape[0]
                                    str_mat = str(map(f_lit, data.flatten().tolist()))
                                    mat_str += str_mat.replace('[', '').replace(']', '') + ','

                                    str_offsets = str([idx[0]] + offsets.flatten().tolist())
                                    offsets_str += str_offsets.replace('[', '').replace(']', '') + ','

                                    sizes_str += str(data_size) + ','
                                    size += data_size
                            else:
                                    sizes_str += '0,'

                    mat_str = mat_str[:-1]
                    offsets_str = offsets_str[:-1]
                    sizes_str = sizes_str[:-1]
                    layers = mat.shape[0]

                    contents += '#define ' + mat_name.upper() + '_LEN ' + str(size) + '\n\n'

                    contents += '__nvram fixed ' + mat_name + \
                            '[' + str(size) + '] = {' + mat_str + '};\n\n'

                    contents += '__nvram fixed ' + mat_name + '_offsets[' + \
                            str(size) + '] = {' + offsets_str + '};\n\n'

                    contents += '__nvram fixed ' + mat_name + '_sizes[' + \
                            str(layers) + '] = {' + sizes_str + '};\n\n'

            elif layer == 'FC' and sparse:
                    csr = scipy.sparse.csr_matrix(mat)
                    data, indices, indptr = csr.data, csr.indices, csr.indptr
                    mat_str = str(map(f_lit, data.flatten().tolist()))
                    mat_str = mat_str.replace('[', '{').replace(']', '}')
                    indices_str = str(indices.flatten().tolist())
                    indices_str = indices_str.replace('[', '{').replace(']', '}')
                    indptr_str = str(indptr.flatten().tolist())
                    indptr_str = indptr_str.replace('[', '{').replace(']', '}')

                    contents += '#define ' + mat_name.upper() + '_LEN ' + \
                            str(len(data)) + '\n\n'

                    contents += '__nvram fixed ' + mat_name + '[' + \
                            str(len(data)) + '] = ' + mat_str + ';\n\n'

                    contents += '__nvram uint16_t ' + mat_name + '_offsets[' + \
                            str(len(indices)) + '] = ' + indices_str + ';\n\n'

                    contents += '__nvram uint16_t ' + mat_name + '_sizes[' + \
                            str(len(indptr)) + '] = ' + indptr_str + ';\n\n'
            elif layer == 'INT':
                    mat_str = str(mat.flatten().tolist())
                    mat_str = mat_str.replace('[', '{').replace(']', '}')
                    shape_str = ''
                    for s in mat.shape:
                            shape_str += '[' + str(s) + ']'

                    contents += '__nvram uint16_t ' + mat_name + \
                            shape_str + ' = ' + mat_str + ';\n\n'
            elif layer == 'JAGGED_INT':
                    lmat = mat.tolist()
                    pointer_str = ''
                    for i, mat in enumerate(lmat):
                            nmat = np.array(mat)
                            mat_str = str(nmat.flatten().tolist())
                            mat_str = mat_str.replace('[', '{').replace(']', '}')
                            shape_str = ''
                            for s in nmat.shape:
                                    shape_str += '[' + str(s) + ']'

                            contents += '__nvram uint16_t ' + mat_name + '_' + str(i) + \
                                    shape_str + ' = ' + mat_str + ';\n\n'
                            pointer_str += '%s_%d,' %(mat_name, i)

                    pointer_str = pointer_str[:-1]
                    contents += '__nvram uint16_t *%s[%d] = {%s};\n\n' % (
                            mat_name, len(lmat), pointer_str)
            elif layer == 'JAGGED_FC':
                    lmat = mat.tolist()
                    pointer_str = ''
                    for i, mat in enumerate(lmat):
                            nmat = np.array(mat)
                            mat_str = str(list(map(f_lit, nmat.flatten().tolist())))
                            mat_str = mat_str.replace('[', '{').replace(']', '}')
                            shape_str = ''
                            for s in nmat.shape:
                                    shape_str += '[' + str(s) + ']'

                            contents += 'fixed __attribute__ ((section(".upper.rodata"))) ' + mat_name + '_' + str(i) + \
                                    shape_str + ' = ' + mat_str + ';\n\n'
                            pointer_str += '%s_%d,' %(mat_name, i)

                    pointer_str = pointer_str[:-1]
                    #contents += '__nvram uint16_t *%s[%d] = {%s};\n\n' % (
                    #        mat_name, len(lmat), pointer_str)
            else:
                    mat_str = str(map(f_lit, mat.flatten().tolist()))
                    mat_str = mat_str.replace('[', '{').replace(']', '}')
                    shape_str = ''
                    for s in mat.shape:
                            shape_str += '[' + str(s) + ']'

                    contents += '__nvram fixed ' + mat_name + \
                            shape_str + ' = ' + mat_str + ';\n\n'

    contents = contents.replace("'", '')
    contents += '#endif'
    path = os.path.join(header_dir, name + '.h')
    with open(path, 'w+') as f:
        f.write(contents)

VL = 16
VLs = [16, 32, 64]
F_N = 5
F_ONE = 1 << F_N
to_fixed = lambda x: int(x * F_ONE)
to_float = lambda x: float(x) / F_ONE


def main(args):
    #input_file = sys.argv[1];
    global header_dir
    header_dir = args.header_dir

    start = time.time()

    out = open( "img-input" , 'w')

    hog_features = []

    files = glob.glob("./*.txt")
    #labels = [ 1, 1, 2, 1, 2, 2, 1, 1, 2, 1, 1, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 1 ]
    labels = [ 1, 1, 2, 1, 2, 2, 1, 1, 2, 1, 1, 2, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 1 ]

    count = 0

    hog_test = []

    for input_file in files:
        file = open(input_file, 'r')
        #print('Input File: ' + input_file + " --- " + str(labels[count]));
        string = file.read()

        nstring = np.fromstring(string, dtype=np.uint8, sep=' ')
        nstring = np.resize(nstring, (120, 160))
    #   print(str(nstring.shape))
        fd, hog_image = hog(nstring, orientations=9, pixels_per_cell=(8, 8), cells_per_block=(2, 2), block_norm = 'L2', visualize=True, multichannel=False)
        hog_features.append(fd)
        if count == 0:
            hog_test.append(fd)
        #out.write(str(labels[count]) + " ")
        #for i in range(0, len(fd)):
        #    out.write(str(i+1) + ":" + str(fd[i]) + " ")
        #out.write("\n")
        count += 1

    hog_features = np.array(hog_features)
    print("Hog Shape = " + str(np.shape(hog_features)))

    #labels = [ 1, 1, 1, 1, 1, 2, 2, 2, 2, 1]
    labels =  np.array(labels).reshape(len(labels),1)

    data_frame = np.hstack((hog_features,labels))
    np.random.shuffle(data_frame)

    percentage = 80
    partition = int(len(hog_features)*percentage/100)

    x_train, x_test = data_frame[:partition, :-1],  data_frame[partition:, :-1]
    y_train, y_test = data_frame[:partition, -1:].ravel() , data_frame[partition:, -1:].ravel()

    print("X_train = " + str(np.shape(x_train)))
    print("X_test = " + str(np.shape(x_test)))
    print("y_train = " + str(np.shape(y_train)))
    print("y_test = " + str(np.shape(y_test)))

    clf = SGDClassifier(loss='hinge', penalty='l2')
    clf.fit(x_train, y_train)

    '''
    joblib.dump(clf, 'my_model.pkl', compress=9)


    print("Support Vectors Indices " + str(clf.support_))
    print("n_support " + str(clf.n_support_))
    print("Dual_coef " + str(np.shape(clf.dual_coef_)))
    print(str(clf.dual_coef_))
          
    np.savetxt(out, clf.dual_coef_)
    '''
    y_pred = clf.predict(hog_features)

    print(str(confusion_matrix(labels, y_pred)))

    print('weights: ' + str(clf.coef_.shape))
    print('bias: ' + str(clf.intercept_.shape))
    
    #weight = np.round(clf.coef_, 2)
    weight = clf.coef_

    print(str(weight))
    print(str(weight[0]))

    write_header('svm', [('svm_w', weight, 'JAGGED_FC', False), ('svm_b', clf.intercept_, 'JAGGED_FC', False)], out)
    

    print("Y_test : " + str(labels))
    print("Y_pred : " + str(y_pred))
    out.write(str(labels) + "\n")
    out.write(str(y_pred) + "\n")

    print("Accuracy: "+str(100*accuracy_score(labels, y_pred)) + " %")
    print("\n")
    print(classification_report(labels, y_pred))
    out.write(str(100*accuracy_score(labels, y_pred)) + " % \n")
    out.write(str(classification_report(labels, y_pred)) + "\n")
    out.wr
    decis = clf.decision_function(hog_features)
    print(str(decis))
    out.write(str(decis))
   
    print(str(len(hog_features)))
    hog_test = np.array(hog_test)
    hog_test = np.transpose(hog_test)
    hog_test = hog_test.reshape(9576, )
    print(str(np.shape(hog_test)))
   
    weight = np.transpose(weight)
    weight = weight.reshape(9576, )
    print(str(np.shape(weight)))
    
    weight2 = np.transpose(clf.coef_)
    weight2 = weight2.reshape(9576, )
    print(str(np.shape(weight)))
    
    res = 0
    res2 = 0
    for i in range(0, len(hog_test)):
        res += hog_test[i] * weight[i]
        res2 += hog_test[i] * weight2[i]

    res = res+clf.intercept_
    res2 = res2+clf.intercept_
    print("res = " + str(res))
    print("res2 = " + str(res2))

    vars_to_save = {'fc1_w.summary': clf.coef_, 'fc1_b.summary': clf.intercept_,}
            #'embeddings_w.summary': rweights, 'embeddings_b.summary': roffsets}
    
    end = time.time()
    print(end - start)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--header_dir',
		type=str,
		default='headers',
		help='Generated Header(s) Directory')
	args = parser.parse_args()
	main(args)

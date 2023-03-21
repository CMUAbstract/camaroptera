import tensorflow as tf
import numpy as np
from PIL import ImageOps
from keras.preprocessing.image import load_img
import sys

fname = sys.argv[1]
img = load_img(fname)

gimg = ImageOps.grayscale(img)

image = np.array(gimg)
image = image / 255.
image = image.reshape(1,160,120,1)

model = tf.keras.models.load_model('testbench/tf-train/camaroptera-model')

pred = model.predict(image)

print(pred)

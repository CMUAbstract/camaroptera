import tensorflow as tf
import numpy as np
#import matplotlib.pyplot as plt
from tensorflow import keras
from tensorflow.keras import layers
from keras.preprocessing.image import ImageDataGenerator

#put image loading code here
train_data_dir = 'testbench/tf-train/train'
validation_data_dir = 'testbench/tf-train/validate'

nb_train_samples = 400
nb_validation_samples = 100

img_width = 160
img_height = 120
num_classes = 3

epochs = 10
batch_size = 16

input_shape = (img_width,img_height,1)
model = keras.Sequential(
    [
        keras.Input(shape=input_shape),
        layers.Conv2D(32, kernel_size=(3, 3), activation="relu"),
        layers.MaxPooling2D(pool_size=(2, 2)),
        layers.Conv2D(64, kernel_size=(3, 3), activation="relu"),
        layers.MaxPooling2D(pool_size=(2, 2)),
        layers.Flatten(),
        layers.Dropout(0.5),
        layers.Dense(num_classes, activation="softmax"),
    ]
)

model.compile(optimizer='adam',
              loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True),
              metrics=['accuracy'])

train_datagen = ImageDataGenerator(rescale=1. / 255)

test_datagen = ImageDataGenerator(rescale=1. / 255)

train_generator = train_datagen.flow_from_directory(
    train_data_dir,
    color_mode="grayscale",
    target_size=(img_width, img_height),
    batch_size=batch_size,
    class_mode='categorical')
 
validation_generator = test_datagen.flow_from_directory(
    validation_data_dir,
    color_mode="grayscale",
    target_size=(img_width, img_height),
    batch_size=batch_size,
    class_mode='categorical')
 
model.fit_generator(
    train_generator,
    steps_per_epoch=nb_train_samples // batch_size,
    epochs=epochs,
    validation_data=validation_generator,
    validation_steps=nb_validation_samples // batch_size)

model.save('testbench/tf-train/camaroptera-model')

#train_images = train_images / 255.0
#test_images = test_images / 255.0

#model.fit(train_images, train_labels, epochs=10)

#test_loss, test_acc = model.evaluate(test_images, test_labels, verbose=2)
#print('\nTest Accuracy:', test_acc)
#
#prob_mod = tf.keras.Sequential([model, tf.keras.layers.Softmax()])
#predictions = prob_mod.predict(test_images)
#predictions[0]
#np.argmax(predictions[0])
#
#img = test_images[1]
#img = (np.expand_dims(img,0))
#pred_sing = prob_mod.predict(img)
#print(pred_sing)
#print(test_labels[1])

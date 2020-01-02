from __future__ import absolute_import, division, print_function, unicode_literals
import sys
import os
import tensorflow as tf
from tensorflow.keras import datasets, layers, models
from tensorflow.keras.models import load_model
import matplotlib.pyplot as plt
import numpy as np

from maraboupy import MarabouParseCnn as mcnn

if len(sys.argv) > 1:
    model_file = str(sys.argv[1])
    from_file = True
else:
    model_file = './cnn_model.h5'
    from_file  = os.path.isfile(model_file)
    

    
print(from_file)

if not from_file:


    (train_images, train_labels), (test_images, test_labels) = datasets.cifar10.load_data()

    # Normalize pixel values to be between 0 and 1
    train_images, test_images = train_images / 255.0, test_images / 255.0

    class_names = ['airplane', 'automobile', 'bird', 'cat', 'deer',
               'dog', 'frog', 'horse', 'ship', 'truck']

    plt.figure(figsize=(10,10))
    for i in range(25):
        plt.subplot(5,5,i+1)
        plt.xticks([])
        plt.yticks([])
        plt.grid(False)
        plt.imshow(train_images[i], cmap=plt.cm.binary)
        # The CIFAR labels happen to be arrays, 
        # which is why you need the extra index
        plt.xlabel(class_names[train_labels[i][0]])
    plt.show()

    model = models.Sequential()
    model.add(layers.Conv2D(32, (3, 3), activation='relu', input_shape=(32, 32, 3)))
    model.add(layers.MaxPooling2D((2, 2)))
    model.add(layers.Conv2D(64, (3, 3), activation='relu'))
    model.add(layers.MaxPooling2D((2, 2)))
    model.add(layers.Conv2D(64, (3, 3), activation='relu'))

    model.add(layers.Flatten())
    model.add(layers.Dense(64, activation='relu'))
    model.add(layers.Dense(10, activation='softmax'))

    model.summary()

    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])

    print(np.shape(train_images))
    print(np.shape(train_labels))
    print(np.shape(test_images))
    print(np.shape(test_labels))

    history = model.fit(train_images, train_labels, epochs=10, validation_data=(test_images, test_labels))

#   plt.plot(history.histoy['accuracy'], label='accuracy')
#   plt.plot(history.history['val_accuracy'], label = 'val_accuracy')
#   plt.xlabel('Epoch')
#   plt.ylabel('Accuracy')
#   plt.ylim([0.5, 1])
#   plt.legend(loc='lower right')

    test_loss, test_acc = model.evaluate(test_images,  test_labels, verbose=2)
    
    model.summary()
    model.save('cnn_model.h5')

else:
    model = load_model(model_file)


nx_cnn = mcnn.Cnn(mcnn.Cnn.keras_to_nx())
nx_cnn.print_to_file("cnn_print_nx.png")


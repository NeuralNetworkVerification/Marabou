import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product
#pip install keras2onnx
import tensorflow as tf
#from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import numpy as np

import copy

print("Starting model building")
#https://keras.io/examples/vision/mnist_convnet/

# Model / data parameters
num_classes = 10
input_shape = (28 * 28,1)

# the data, split between train and test sets
(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

# Scale images to the [0, 1] range
x_train = x_train.astype("float32") / 255
x_test = x_test.astype("float32") / 255
# Make sure images have shape (28, 28, 1)
x_train = x_train.reshape(x_train.shape[0], 28 * 28)
x_test = x_test.reshape(x_test.shape[0], 28 * 28)
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)
print("x_train shape:", x_train.shape)
print(x_train.shape[0], "train samples")
print(x_test.shape[0], "test samples")
    
# convert class vectors to binary class matrices
y_train = tf.keras.utils.to_categorical(y_train, num_classes)
y_test = tf.keras.utils.to_categorical(y_test, num_classes)

model = tf.keras.Sequential(
    [
        tf.keras.Input(shape=input_shape),
        layers.Conv1D(32, kernel_size=(3,), activation="relu", name="c1"),
        layers.MaxPooling1D(pool_size=(2,), name="mp1"),
        layers.Conv1D(64, kernel_size=(3,), activation="relu", name="c2"),
        layers.MaxPooling1D(pool_size=(2,), name="mp2"),
        layers.Flatten(name="f1"),
        layers.Dropout(0.5, name="do1"),
        layers.Dense(num_classes, activation="softmax", name="sm1"),
    ]
)

model.summary()

#batch_size = 128
batch_size = 60
epochs = 1

model.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

model.fit(x_train, y_train, batch_size=batch_size, epochs=epochs, validation_split=0.1)

score = model.evaluate(x_test, y_test, verbose=0)
print("Test loss:", score[0])
print("Test accuracy:", score[1])

c2 = model.get_layer(name="c2")
c2out = c2.output_shape
replace_dense = layers.Dense(units=np.prod(c2out[1:]),name="dReplace")

print("Clone model")
modelAbs = tf.keras.Sequential(
    [
        tf.keras.Input(shape=input_shape),
        model.get_layer(name="c1"),
        model.get_layer(name="mp1"),
        layers.Flatten(name="FlattenReplace"),
        replace_dense,
        layers.Reshape(model.get_layer(name="c2").output_shape[1:], name="reshapeReplace"),
        #model.get_layer(name="c2"),
        model.get_layer(name="mp2"),
        model.get_layer(name="f1"),
        model.get_layer(name="do1"),
        model.get_layer(name="sm1")
    ]
)

modelAbs.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
modelAbs.summary()

orig_w = model.get_layer(name="c2").get_weights()[0]
orig_b = model.get_layer(name="c2").get_weights()[1]

replace_w = np.zeros(replace_dense.get_weights()[0].shape)
mask = np.ones(c2out[1:-1]) #Mask is not considering different channels

print("Start process")
strides = c2.strides
filter_dim = orig_w.shape[0:-2]
soff = [np.prod(orig_w.shape[i+1:-1]) for i in range(len(orig_w.shape[:-2]))] + [1]
toff = [np.prod(c2out[i+2:]) for i in range(len(c2out)-2)] + [1]
'''print(orig_w.shape)
print(filter_dim)
print(soff)
print(c2out)
print(toff)'''
for target_coor in product(*[range(d) for d in c2out[1:-1]]):       
    if mask[target_coor]:
        for source_coor, wMat in zip(product(*[[i*s+t for i in range(d)] for d,s,t in zip(filter_dim, strides, target_coor)]), [orig_w[c] for c in product(*[range(d) for d in filter_dim])]):
            for in_ch, out_ch in product(range(orig_w.shape[-2]), range(orig_w.shape[-1])):
                flat_s = sum([c * off for c,off in zip((*source_coor, in_ch) , soff)])
                flat_t = sum([c * off for c,off in zip((*target_coor, out_ch), toff)])
                if in_ch == 0 and out_ch == 0:
                    '''print("coordinates: source_coor={}, in_ch={}, flat_s={}, target_coor={}, out_ch={}, flat_t={}".format(source_coor,in_ch, flat_s,target_coor, out_ch, flat_t))
                    print(replace_w.shape)
                    print(wMat.shape)
                    if flat_s == 0:
                        print("Flat_s is 0, soff={}, toff={}".format(soff, toff))'''
                replace_w[flat_s, flat_t] = 1 # wMat[in_ch, out_ch]
    

#replace_b = np.tile(orig_b, np.prod(c2out[1:-1])) TODO return this is real code
replace_b = np.tile(np.ones(orig_b.shape), np.prod(c2out[1:-1]))
replace_dense.set_weights([replace_w, replace_b])
#print("After Change")
#print([np.all(w1==w2) for w1,w2 in zip(replace_dense.get_weights(), modelAbs.get_layer(name="dReplace").get_weights())])
#print("Weights")
#print(modelAbs.get_layer(name="dReplace").get_weights())

#score = modelAbs.evaluate(x_test, y_test, verbose=0)
#print("Test loss:", score[0])
#print("Test accuracy:", score[1])

#evals = modelAbs.predict(x_test) == model.predict(x_test)
#if np.all(evals):
#    print("Prediction aligned")    
#else:
#    print("Prediction not aligned")
#    print(evals)

print("Evaluating")
c2replaced = model.get_layer(name="c2")
#c2replaced.set_weights([np.ones(c2replaced.get_weights()[0].shape), c2replaced.get_weights()[1]]) #0.7
c2replaced.set_weights([np.ones(c2replaced.get_weights()[0].shape), np.ones(c2replaced.get_weights()[1].shape)])
slice_input_shape = c2replaced.input_shape[1:]
origModelSlice = tf.keras.Sequential(
    [
        tf.keras.Input(shape=slice_input_shape),
        #model.get_layer(name="c1"),
        #model.get_layer(name="mp1"),
        c2replaced
    ]
)
origModelSlice.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
origModelIn = tf.keras.Sequential(
    [
        tf.keras.Input(shape=input_shape),
        model.get_layer(name="c1"),
        model.get_layer(name="mp1"),
        #c2replaced
    ]
)
origModelIn.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
print("Created orig slice")
modelAbsSlice = tf.keras.Sequential(
    [
        tf.keras.Input(shape=slice_input_shape),
        #modelAbs.get_layer(name="c1"),
        #modelAbs.get_layer(name="mp1"),
        modelAbs.get_layer(name="FlattenReplace"),
        modelAbs.get_layer(name="dReplace"),
        modelAbs.get_layer(name="reshapeReplace")
    ]
)
modelAbsSlice.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
print("Created abs slice")

'''slice_test = np.array([
    np.ones(slice_input_shape),
    np.zeros(slice_input_shape)
    ])'''
slice_test = origModelIn.predict(x_test[:10])
evalsSlice = np.isclose(modelAbsSlice.predict(slice_test),origModelSlice.predict(slice_test))
if np.all(evalsSlice):
    print("Slice Prediction aligned")
    print(modelAbsSlice.predict(slice_test))
    print([np.unique(modelAbsSlice.predict(slice_test))])
else:
    print("Slice Prediction not aligned")
    print(evalsSlice)
    print(np.mean([1 if b else 0 for b in np.nditer(evalsSlice)]))

    for origM, absM, e in zip(np.nditer(origModelSlice.predict(slice_test)), np.nditer(modelAbsSlice.predict(slice_test)), np.nditer(evalsSlice)):
        if not e:
            if not np.isclose(origM, absM):
                print("False:")
                print("orig: {} = {}".format(origM, np.round(origM, 4)))
                print("abs:  {} = {}".format(absM,  np.round(absM, 4)))



#[print(w.shape) for w in  model.get_layer(name="c2").get_weights()]
#print("Dense")
#[print(w.shape) for w in  modelAbs.get_layer(name="dReplace").get_weights()]

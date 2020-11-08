import larq as lq
import tensorflow as tf
import larq_zoo as lqz
from maraboupy import Marabou

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import sklearn


PATH_1 = "/cs/labs/guykatz/guyam/test_larq_pycharm"
PATH_2 = "/cs/labs/guykatz/guyam/test_leon"


PATH_tiny_BNN = "/cs/labs/guykatz/guyam/test_tiny_BNN"
outputName = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"


PATH_tiny_two_layer = "/cs/labs/guykatz/guyam/models_to_test/tiny_two_layer"
outputName_tiny_two_layer = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"


PATH_tiny_one_layer = "/cs/labs/guykatz/guyam/models_to_test/tiny_one_layer"
outputName_tiny_one_layer = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_1/FC_out_10/MatMul"


def load_data():
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    # Rescale the images from [0,255] to the [0.0,1.0] range.
    x_train, x_test = x_train / 255.0, x_test / 255.0

    print("Number of original training examples:", len(x_train))
    print("Number of original test examples:", len(x_test))

    return x_train, y_train, x_test, y_test

# change dimension of input to vecotrs of the size 784

def change_input_dimension(X):
    return X.reshape((X.shape[0],X.shape[1]*X.shape[2]))



def check_if_saved_weights_binary():
    # test block - data loaded ok
    x_train, y_train, x_test, y_test = load_data()

    # test block - new dimension is a vector
    x_train = x_train.reshape((x_train.shape[0],x_train.shape[1]*x_train.shape[2]))
    x_test = x_test.reshape((x_test.shape[0],x_test.shape[1]*x_test.shape[2]))

    # the kernel_quantizer makes the weiths actially +-1
    kwargs = dict(input_quantizer="ste_sign",kernel_quantizer="ste_sign", kernel_constraint="weight_clip")

    # recommended (Larq) batch norm momentum
    bn_momentum = 0.25 # was 0.9 when using Bop

    # number of epochs
    epoch_num = 1 # was 50

    model = tf.keras.models.Sequential()


    model.add(tf.keras.layers.Input(shape=[784]))

    model.add(tf.keras.layers.BatchNormalization(scale=False, momentum=bn_momentum, name="BN_1"))
    model.add(lq.layers.QuantDense(units=500, use_bias=False, **kwargs, name="FC_out_500")) # was input_kwargs

    model.add(tf.keras.layers.BatchNormalization(scale=False, momentum=bn_momentum, name="BN_2"))
    model.add(lq.layers.QuantDense(units=300, use_bias=False, **kwargs, name="FC_out_300"))

    model.add(tf.keras.layers.BatchNormalization(scale=False, momentum=bn_momentum, name="BN_3"))
    model.add(lq.layers.QuantDense(units=200, use_bias=False, **kwargs, name="FC_out_200"))

    model.add(tf.keras.layers.BatchNormalization(scale=False, momentum=bn_momentum, name="BN_4"))
    model.add(lq.layers.QuantDense(units=100, use_bias=False, **kwargs, name="FC_out_100"))

    model.add(tf.keras.layers.BatchNormalization(scale=False,momentum=bn_momentum, name="BN_5"))
    model.add(lq.layers.QuantDense(units=10, use_bias=False, **kwargs, name="FC_out_10"))

    # output layer
    model.add(tf.keras.layers.Activation("softmax"))

    # logits loss function
    loss_fn = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)
    optimizer = tf.keras.optimizers.Adam()


    model.compile(loss=loss_fn, optimizer=optimizer, metrics=['accuracy'])

    lq.models.summary(model)

    # train model & check test accuracy
    model.fit(x=x_train, y=y_train, validation_data=(x_test, y_test), epochs=epoch_num, verbose=True)

    with lq.context.quantized_scope(True):
        # model.save("TempSavedModel")
        model.save(PATH_1)
        # print(model.get_weights()) # this does print binary

    loaded = tf.saved_model.load(PATH_1) # kyle mraboupy load?
    loaded2 = tf.keras.models.load_model(PATH_1) # leon larq load



def leon_check():
    # Load model and save binary weights
    model_leon = lqz.sota.QuickNet()

    with lq.context.quantized_scope(True):
        model_leon.save(PATH_2)
        print(model_leon.get_weights()) # this prints binary weights
        a=1
    print("****")
    # Reload model and print weights
    loaded_2 = tf.keras.models.load_model(PATH_2)
    loaded = tf.saved_model.load(PATH_2) # kyle mraboupy load?
    # i=1
    for idx,layer in enumerate(loaded_2.layers):
        print("layer num " + str(idx))
        print(layer.weights)
        # i+=1

        # a=3
        # print(type(layer))
        # if isinstance(layer, lq.layers.QuantConv2D):
        # # if type(layer) == tf.python.keras.saving.saved_model.load.QuantConv2D:
        #     print("sdfs")
    a=2


def train_tiny_bnn():
    # test block - data loaded ok
    x_train, y_train, x_test, y_test = load_data()

    # test block - new dimension is a vector
    x_train = x_train.reshape((x_train.shape[0],x_train.shape[1]*x_train.shape[2]))
    x_test = x_test.reshape((x_test.shape[0],x_test.shape[1]*x_test.shape[2]))

    kwargs = dict(input_quantizer="ste_sign", kernel_quantizer="ste_sign",  kernel_constraint="weight_clip")
    bn_momentum = 0.25
    epoch_num = 1

    model = tf.keras.models.Sequential()
    model.add(tf.keras.layers.Input(shape=[784])) # input layer
    model.add(tf.keras.layers.BatchNormalization(scale=False, momentum=bn_momentum, name="BN_1"))

    model.add(lq.layers.QuantDense(units=30, use_bias=False, **kwargs, name="FC_out_30"))
    model.add(tf.keras.layers.BatchNormalization(scale=False,momentum=bn_momentum, name="BN_2"))
    model.add(lq.layers.QuantDense(units=10, use_bias=False, **kwargs, name="FC_out_10"))
    model.add(tf.keras.layers.Activation("softmax")) # output layer

    loss_fn = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)
    optimizer = tf.keras.optimizers.Adam()
    model.compile(loss=loss_fn, optimizer=optimizer, metrics=['accuracy'])
    model.fit(x=x_train, y=y_train, validation_data=(x_test, y_test), epochs=epoch_num, verbose=True)
    return model









if __name__ == '__main__':
    # model = train_tiny_bnn()
    # with lq.context.quantized_scope(True):
    #     model.set_weights(model.get_weights())  # remove latent weights
    #     model.save(PATH_3) # save model


    loaded_leon = tf.keras.models.load_model(PATH_tiny_BNN) # load the model
    loaded_kyle = tf.saved_model.load(PATH_tiny_BNN) # kyle mraboupy load?

    net = Marabou.read_tf(filename=PATH_tiny_BNN, modelType="savedModel_v2", outputName=outputName)

    print("yo")


    # relevant code copied from MarabouTF.py

    # from tensorflow.python.framework.convert_to_constants import convert_variables_to_constants_v2
    # savedModelTags = ["serving_default"]
    # loaded = tf.keras.models.load_model(PATH_3) # load the model
    #
    # # Read SavedModel format created by tensorflow version 2.X
    # sess = tf.compat.v1.Session()
    # model = loaded.signatures[savedModelTags[0]]
    #
    # # Create a concrete function from model
    # full_model = tf.function(lambda x: model(x))
    # full_model = full_model.get_concrete_function(tf.TensorSpec(model.inputs[0].shape, model.inputs[0].dtype))
    #
    # # Get frozen ConcreteFunction
    # frozen_func = convert_variables_to_constants_v2(full_model)
    # sess = tf.compat.v1.Session(graph=frozen_func.graph)
    #
    #
    #
    # # Valid names to use for input or output operations
    # valid_names = [op.node_def.name for op in sess.graph.get_operations()]
    #
    # print('ya')





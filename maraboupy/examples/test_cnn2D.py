from maraboupy import MarabouNetworkNX as mnx
from maraboupy import MarabouParseCnn as mcnn
from itertools import combinations, product
from math  import ceil
import numpy as np
import matplotlib.pyplot as plt
import random

import sys
import os
import tensorflow as tf
from tensorflow.keras import datasets, layers, models
from tensorflow.keras.models import load_model
import re

def create_cnn2D():
    in_l_size = {"x":8 , "y":8, "d":2 }
    
    #3 X 4 X 2 X 2
    f_1_l = np.array([[ [ [1.111 , 1.112] , [1.121 , 1.122] ] , [ [1.211 , 1.212] , [1.221 , 1.222] ] , [ [1.311 , 1.312] , [1.321 , 1.322] ] , [ [1.411 , 1.412] , [1.421 , 1.422] ] ],
                      [ [ [2.111 , 2.112] , [2.121 , 2.122] ] , [ [2.211 , 2.212] , [2.221 , 2.222] ] , [ [2.311 , 2.312] , [2.321 , 2.322] ] , [ [2.411 , 2.412] , [2.421 , 2.422] ] ],
                      [ [ [3.111 , 3.112] , [3.121 , 3.122] ] , [ [3.211 , 3.212] , [3.221 , 3.222] ] , [ [3.311 , 3.312] , [3.321 , 3.322] ] , [ [3.411 , 3.412] , [3.421 , 3.422] ] ]])
    f_1 = mcnn.Filter(f_1_l)

    #output : ?x X ?y X 2d
    f_2 = mcnn.Filter([],"MaxPool", [2,2,f_1.dim["d"]])
    
    #2x X 2y X 2d X 2f
    f_3_l = np.array([[ [ [1.111 , 1.112] , [1.121 , 1.122] ] , [ [1.211 , 1.212] , [1.221 , 1.222] ]],
                      [ [ [2.111 , 2.112] , [2.121 , 2.122] ] , [ [2.211 , 2.212] , [2.221 , 2.222] ]]])
    f_3 = mcnn.Filter(f_3_l)

    # ?x X ?y X 2d
    f_4 = mcnn.Filter([],"MaxPool", [1,1,f_3.dim["d"]])

    cnn = mcnn.Cnn2D(in_l_size)
    cnn.add_filter(f_1)
    print("Added f_1")
    cnn.add_filter(f_2)
    print("Added f_2")    
    cnn.add_filter(f_3)
    print("Added f_3")
    cnn.add_filter(f_4)
    print("Added f_4")
    cnn.add_flatten()
    print("Added flatten")
    cnn.add_dense({cor : [((j,0,0),1) for j in range(3)] for cor in cnn.out_l})
    print("Added dense")
    print("Out dim:" + str([k + "=" + str(v) for k,v in cnn.out_dim.items()]))
    
    #for n,v in cnn.nodes.items():
    #    print(str(n) + ":" + (v["function"] if "function" in v else ""))
    #for i,e in enumerate(sorted(cnn.edges, key= lambda e : e[::-1])):
    #    print(str(i) + ":" + str(e) + ":" + str(cnn.edges[e]["weight"]))

    return cnn

def create_min_cnn2D():
    in_l_size = {"x":3 , "y":3, "d":1 }
    
    #3 X 4 X 2 X 2
    f_1_l = np.array([[ [ [1.111 , 1.112] , [1.121 , 1.122] ] , [ [1.211 , 1.212] , [1.221 , 1.222] ] ],
                      [ [ [2.111 , 2.112] , [2.121 , 2.122] ] , [ [2.211 , 2.212] , [2.221 , 2.222] ] ]])
    f_1 = mcnn.Filter(f_1_l)

    #output : ?x X ?y X 2d
    f_2 = mcnn.Filter([],"MaxPool", [1,1,f_1.dim["d"]])

    cnn = mcnn.Cnn2D(in_l_size)
    cnn.add_filter(f_1)
    print("Added f_1")
    cnn.add_filter(f_2)
    print("Added f_2")    
    cnn.add_flatten()
    print("Added flatten")
    cnn.add_dense({cor : [((j,0,0),1) for j in range(4)] for cor in cnn.out_l})
    print("Added dense")
    print("Out dim:" + str([k + "=" + str(v) for k,v in cnn.out_dim.items()]))
    
    #for n,v in cnn.nodes.items():
    #    print(str(n) + ":" + (v["function"] if "function" in v else ""))
    #for i,e in enumerate(sorted(cnn.edges, key= lambda e : e[::-1])):
    #    print(str(i) + ":" + str(e) + ":" + str(cnn.edges[e]["weight"]))

    return cnn


def train_cnn2D():
    file_name = './cnn_model.h5'
    
    (train_images, train_labels), (test_images, test_labels) = datasets.cifar10.load_data()

    # Normalize pixel values to be between 0 and 1
    train_images, test_images = train_images / 255.0, test_images / 255.0

    class_names = ['airplane', 'automobile', 'bird', 'cat', 'deer', 'dog', 'frog', 'horse', 'ship', 'truck']

    model = models.Sequential()
    model.add(layers.Conv2D(2, (3, 3), activation='relu', input_shape=(32, 32, 3)))    
    #model.add(layers.Conv2D(32, (3, 3), activation='relu', input_shape=(32, 32, 3)))
    model.add(layers.MaxPooling2D((2, 2)))
    model.add(layers.Conv2D(2, (3, 3), activation='relu'))
    #model.add(layers.Conv2D(64, (3, 3), activation='relu'))
    model.add(layers.MaxPooling2D((2, 2)))
    model.add(layers.Conv2D(2, (3, 3), activation='relu')) 
    #model.add(layers.Conv2D(64, (3, 3), activation='relu')) 

    model.add(layers.Flatten())
    #model.add(layers.Dense(64, activation='relu'))TODO
    #model.add(layers.Dense(10, activation='relu'))TODO

    model.summary()
    '''
    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])

    print(np.shape(train_images))
    print(np.shape(train_labels))
    print(np.shape(test_images))
    print(np.shape(test_labels))

    
    history = model.fit(train_images, train_labels, epochs=10, validation_data=(test_images, test_labels))

    plt.plot(history.histoy['accuracy'], label='accuracy')
    plt.plot(history.history['val_accuracy'], label = 'val_accuracy')
    plt.xlabel('Epoch')
    plt.ylabel('Accuracy')
    plt.ylim([0.5, 1])
    plt.legend(loc='lower right')

    test_loss, test_acc = model.evaluate(test_images,  test_labels, verbose=2)    
    model.summary()
    model.save(file_name)'''

    in_l_size = {"x":32 , "y":32, "d":3 }
    cnn = mcnn.Cnn2D(in_l_size)
    
    for layer in model.layers:
        if layer.get_config()["name"].startswith("conv2d"):
            f = mcnn.Filter(layer.weights)
            cnn.add_filter(f)                
        elif layer.get_config()["name"].startswith("max_pooling2d"):
            f = mcnn.Filter([],function="MaxPool", shape=list(layer.get_config()["pool_size"]))
            cnn.add_filter(f)
        elif layer.get_config()["name"].startswith("flatten"):
            cnn.add_flatten()
        elif layer.get_config()["name"].startswith("dense"):
            print("In shape:{}\nOut shape:{}".format(layer.input_shape, layer.output_shape))
            print(layer.weights)
            weights = layer.weights[0].numpy()
            cnn.add_dense({(i,0,0):[((j,0,0), weights[i][j]) for j in range(layer.output_shape[1])] for i in range(layer.input_shape[1])}) #TODO no bias included

    return cnn
            
        
if __name__ == "__main__": 

    #cnn = create_min_cnn2D()
    #cnn = create_cnn2D()
    cnn = train_cnn2D()
    in_prop  = {n : (0.0001,20) for n in cnn.in_l.values()}
    out_prop = {n : (-5, mnx.large) for n in cnn.out_l.values()}
    for i,n in enumerate(cnn.nodes()):
        print("{}:{}".format(i,n))    
    print("Start solving CNN")
    #cnn.solve(in_prop, out_prop) TODO
    print("Finish solving CNN")

    print("*********************************** COI **********************************************")
    print(list(cnn.out_l.values()))
    print("COI node:{}".format(mcnn.n2str_md(cnn.l_num,[0,0,0])))
    coi_cnn = mcnn.Cnn2D.coi(cnn,[mcnn.n2str_md(cnn.l_num,[0,0,0])])
    for i,n in enumerate(coi_cnn.nodes()):
        print("{}:{}".format(i,n))
    in_prop  = {n : (0.0001,20) for n in coi_cnn.in_l.values()}
    out_prop = {n : (-5, mnx.large) for n in coi_cnn.out_l.values()}
    print("Start solving COI CNN")    
    coi_cnn.solve(in_prop, out_prop)
    print("Finish solving COI CNN")        
        

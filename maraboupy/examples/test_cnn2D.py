from maraboupy import MarabouNetworkNX as mnx
from maraboupy import MarabouParseCnn as mcnn
from itertools import combinations, product
from math  import ceil
import numpy as np
import matplotlib.pyplot as plt
import random

from PIL import Image
import sys
import os
import tensorflow as tf
from tensorflow.keras import datasets, layers, models
from tensorflow.keras.models import load_model
import re
import time
import statistics as stat

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
    cnn.add_filter(f_2)
    cnn.add_filter(f_3)
    cnn.add_filter(f_4)
    cnn.add_flatten()
    cnn.add_dense({cor : [((j,0,0),1) for j in range(3)] for cor in cnn.out_l})    
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
    cnn.add_filter(f_2)
    cnn.add_flatten()
    cnn.add_dense({cor : [((j,0,0),1) for j in range(4)] for cor in cnn.out_l})
    return cnn


def train_cnn2D():
    file_name = './cnn_model.h5'
    
    (train_images, train_labels), (test_images, test_labels) = datasets.cifar10.load_data()

    # Normalize pixel values to be between 0 and 1
    train_images, test_images = train_images / 255.0, test_images / 255.0

    class_names = ['airplane', 'automobile', 'bird', 'cat', 'deer', 'dog', 'frog', 'horse', 'ship', 'truck']

    if isinstance(train_images[0], Image):
        print("Yes")
    else:
        print("No")
    exit()
    x = 16
    y = 16
    d = 3
    input_shape_crop = (x,y,d) #(32, 32, 3)
    train_images_crop = list()
    test_images_crop = list()
    for im in train_images:
        train_images_crop.append(im.crop(0,0,x,y))
        #train_images_crop.append([[[im[x_i][y_i][d_i] for d_i in range(d)] for y_i in range(y)] for x_i in range(x)])
    for im in test_images:
        test_images_crop.append(im.crop(0,0,x,y))        
        #test_images_crop.append([[[im[x_i][y_i][d_i] for d_i in range(d)] for y_i in range(y)] for x_i in range(x)])
    
    #exit() #TODO
    
    model = models.Sequential()
    model.add(layers.Conv2D(1, (4, 4), activation='relu', input_shape=input_shape_crop))    
    ###model.add(layers.Conv2D(32, (3, 3), activation='relu', input_shape=(32, 32, 3)))
    model.add(layers.MaxPooling2D((4, 4)))
    #model.add(layers.Conv2D(2, (3, 3), activation='relu'))
    ###model.add(layers.Conv2D(64, (3, 3), activation='relu'))
    #model.add(layers.MaxPooling2D((2, 2)))
    #model.add(layers.Conv2D(2, (3, 3), activation='relu')) 
    ###model.add(layers.Conv2D(64, (3, 3), activation='relu')) 

    model.add(layers.Flatten())
    #model.add(layers.Dense(64, activation='relu'))
    #model.add(layers.Dense(10, activation='relu'))
    model.summary()

    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])    
    history = model.fit(train_images_crop, train_labels, epochs=10, validation_data=(test_images_crop, test_labels))
    #plt.plot(history.histoy['accuracy'], label='accuracy')
    #plt.plot(history.history['val_accuracy'], label = 'val_accuracy')
    #plt.xlabel('Epoch')
    #plt.ylabel('Accuracy')
    #plt.ylim([0.5, 1])
    #plt.legend(loc='lower right')
    #plt.show(block=False)
    test_loss, test_acc = model.evaluate(test_images,  test_labels, verbose=2)    
    model.summary()
    model.save(file_name)

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



################################################################################################            
## Main Script #################################################################################
################################################################################################

if __name__ == "__main__": 

    print("Start generating CNN")
    cnn_orig = train_cnn2D()
    print("Finish Generating CNN")

    num_of_samples = 3
    sample_size = 3
    samples = [random.sample(set(cnn_orig.out_l.values()), k=sample_size) for i in range(num_of_samples)]
    input_range          = (-mnx.large, mnx.large)
    sample_output_range  = (-5        , mnx.large)
    general_output_range = (-mnx.large, mnx.large)
    out_prop = lambda graph: {n : sample_output_range if n in sample else general_output_range for n in graph.out_l.values()}
    in_prop  = lambda graph: {n : input_range for n in graph.in_l.values()}

    coi_net_nodes = list()
    orig_net_nodes = list()
    coi_net_edges = list()
    orig_net_edges = list()    
    coi_timing = list()
    orig_timing = list()
    
    for j, sample in enumerate(samples):
        
        print("*********************************** COI **********************************************")    
        print("COI nodes:{}".format(str(sample)))
        #coi_cnn = mcnn.Cnn2D.coi(cnn,[mcnn.n2str_md(cnn.l_num,[0,0,0])])
        #in_prop_coi  = {n : (0.0001,20) for n in coi_cnn.in_l.values()}
        print("Start solving COI CNN")
        for v in sample:
            if v not in cnn_orig.nodes():
                raise Exception("{} not in cnn_orig".format(v))
        coi_cnn = mcnn.Cnn2D.coi(cnn_orig, sample)
        start = time.time()        
        coi_cnn.solve(in_prop(coi_cnn), out_prop(coi_cnn))
        end = time.time()
        coi_timing.append(end - start)
        coi_net_nodes.append(len(coi_cnn.nodes()))
        coi_net_edges.append(len(coi_cnn.edges()))
        print("Finish solving COI CNN")

        if j == (len(samples) - 1):
            print("*********************************** Original **********************************************")
            print("Start solving CNN")
            start = time.time()        
            cnn_orig.solve(in_prop(cnn_orig), out_prop(cnn_orig))
            end = time.time()        
            orig_timing.append(end - start)
            orig_net_nodes.append(len(cnn_orig.nodes()))
            orig_net_edges.append(len(cnn_orig.edges()))    
            print("Finish solving CNN")

    if len(orig_timing) == 1:
        orig_timing = orig_timing * len(coi_timing)
    if len(orig_net_nodes) == 1:
        orig_net_nodes = orig_net_nodes * len(coi_net_nodes)
    if len(orig_net_edges) == 1:
        orig_net_edges = orig_net_edges * len(coi_net_edges)
        
    coi_mean = stat.mean(coi_timing)    
    orig_mean = stat.mean(orig_timing)
    coi_variance = stat.variance(coi_timing)
    orig_variance = stat.variance(orig_timing)
    nodes_ratio = [coi/orig for coi, orig in zip(coi_net_nodes, orig_net_nodes)]
    edges_ratio = [coi/orig for coi, orig in zip(coi_net_edges, orig_net_edges)]
    time_ratio  = [coi/orig for coi, orig in zip(coi_timing,    orig_timing)]

    plt.figure(1)
    plt.subplot(131)
    plt.plot(coi_timing, 'rs', orig_timing, 'b^')
    plt.legend(('COI', 'Orig'))
    plt.title('Runtime [seconds]')
    plt.grid(True)
    plt.subplot(132)
    plt.plot(coi_net_nodes, 'rs', orig_net_nodes, 'b^')
    plt.legend(('COI', 'Orig'))    
    plt.title('Nodes')
    plt.grid(True)    
    plt.subplot(133)    
    plt.plot(coi_net_edges, 'rs', orig_net_edges, 'b^')
    plt.legend(('COI', 'Orig'))    
    plt.title('Edges')
    plt.grid(True)
    plt.show(block=False)
    plt.savefig('test_cnn2D_coi_vs_orig_1.png')

    plt.figure(2)
    plt.plot(edges_ratio, 'b', label='Edges')     
    plt.plot(nodes_ratio, 'y', label='Nodes')    
    plt.plot(time_ratio, 'g', label='Runtime')
    plt.title('COI / Orig CNN ratio')
    plt.legend()
    plt.show(block=False)
    plt.savefig('test_cnn2D_coi_vs_orig_2.png')    

    plt.figure(3)
    plt.plot(time_ratio, nodes_ratio, 'r', label='Nodes/Time')
    plt.plot(time_ratio, edges_ratio, 'b', label='Edges/Time')
    plt.title('COI / Orig CNN ratio correlation')
    plt.legend()
    plt.show(block=False)
    plt.savefig('test_cnn2D_coi_vs_orig_3.png')

    with open('test_cnn2D_output.txt', 'w') as file:
        file.write("COI runtime (seconds):{} \n".format(str(coi_timing)))
        file.write("COI number of nodes:{} \n".format(coi_net_nodes))
        file.write("COI number of edges:{} \n".format(coi_net_edges))
        file.write("Orig runtime (seconds):{} \n".format(str(orig_timing)))
        file.write("Orig number of nodes:{} \n".format(orig_net_nodes))
        file.write("Orig number of edges:{} \n".format(orig_net_edges))                
        
    


    

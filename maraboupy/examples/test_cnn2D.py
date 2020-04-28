from __future__ import division
from maraboupy import MarabouNetworkNX as mnx
from maraboupy import MarabouParseCnn as mcnn
from itertools import combinations, product
from math  import ceil
import numpy as np
import matplotlib.pyplot as plt
import random

import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

#from tensorflow.keras import datasets, layers, models, utils
#from tensorflow.keras import backend as K
#from tensorflow.keras.models import load_model

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras.datasets import mnist
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv2D, MaxPooling2D, Flatten, Dense, Dropout
from tensorflow.keras import backend as K

import re
import time
import datetime
import statistics as stat
from pathlib import Path

#####################################
####Toy Examples of CNN construction
#####################################1

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







#####################################
####Train a CNN with MNIST DateSet
#####################################

def train_cnn2D():

    ##################
    ####Get Dataset
    ##################
    
    file_name = './cnn_model.h5'
    
    (train_images, train_labels), (test_images, test_labels) = mnist.load_data()

    ##################
    ####Shape and CFG
    ##################

    bias_active = True #FIXME

    x = train_images[0].shape[0]
    y = train_images[0].shape[1] if len(train_images[0].shape) > 1 else 1
    d = train_images[0].shape[2] if len(train_images[0].shape) > 2 else 1
    input_shape = (x,y,d)

    print("Finished here")

    batch_size = 128
    num_classes = 10
    epochs = 5 #FIXME

    img_rows, img_cols = x, y
    
    if K.image_data_format() == 'channels_first':
        train_images = train_images.reshape(train_images.shape[0], 1, img_rows, img_cols)
        test_images = test_images.reshape(test_images.shape[0], 1, img_rows, img_cols)
        input_shape = (1, img_rows, img_cols)
    else:
        train_images = train_images.reshape(train_images.shape[0], img_rows, img_cols, 1)
        test_images = test_images.reshape(test_images.shape[0], img_rows, img_cols, 1)
        input_shape = (img_rows, img_cols, 1)

        train_images = train_images.astype('float32')
        test_images = test_images.astype('float32')
        train_images /= 255
        test_images /= 255
        print('train_images shape:', train_images.shape)
        print(train_images.shape[0], 'train samples')
        print(test_images.shape[0], 'test samples')

        # convert class vectors to binary class matrices
        train_labels = keras.utils.to_categorical(train_labels, num_classes)
        test_labels = keras.utils.to_categorical(test_labels, num_classes)

    print("Labels shape")
    print(train_labels.shape)
    print(test_labels.shape)    

    ##################
    ####Layers
    ##################
    
    model = Sequential()
    model.add(Conv2D(5, (3, 4), activation='relu', input_shape=input_shape, use_bias=bias_active))
    model.add(MaxPooling2D((2, 2)))
    model.add(Conv2D(5, (2, 2), activation='relu', use_bias=bias_active))
    model.add(MaxPooling2D((2, 2)))
    model.add(Conv2D(1, (2, 5), activation='relu', use_bias=bias_active)) 
    model.add(Flatten())
    model.summary()

    ###################
    ####Fit
    ###################

    '''model.compile(optimizer='adam',
                  loss=keras.losses.categorical_crossentropy,
                  metrics=['accuracy'])
    
    history = model.fit(train_images, train_labels, batch_size=batch_size, epochs=epochs, verbose=1, validation_data=(test_images, test_labels))

    ###################
    ####Evaluate
    ###################
    
    print(history.history)
    plt.plot(history.history['accuracy'], label='accuracy')
    plt.plot(history.history['val_accuracy'], label = 'val_accuracy')
    plt.xlabel('Epoch')
    plt.ylabel('Accuracy')
    plt.ylim([0.5, 1])
    plt.legend(loc='lower right')
    plt.show(block=False) #TODO FIXME save the figure.
    
    test_loss, test_acc = model.evaluate(test_images,  test_labels, verbose=2)
    print('Test loss:', test_loss)
    print('Test accuracy:', test_acc)
    model.summary()
    model.save(file_name) '''

    rand_im = np.random.random([x,y,d])
    rand_label = model.predict(np.array([rand_im]))[0]
    rand_im = None
    rand_label = None
    in_l_size = {"x":x , "y":y, "d":d }

    ###################
    ####Return CNN Obj
    ###################
    
    return mcnn.Cnn2D.keras_model_to_Cnn2D(in_l_size, model), rand_im, rand_label











###################################################
#### Main Test Script
###################################################

if __name__ == "__main__": 

    ###################
    ####Train CNN
    ###################

    target_dir = sys.argv[1]
    target_dir = str(Path(target_dir).resolve())
    mycwd = os.getcwd()
    local_dir = "test_cnn2D_output_dir" + datetime.datetime.now().strftime("_%m-%d-%Y__%H-%M-%S")
    os.chdir(target_dir)
    os.mkdir(local_dir)
    os.chdir(target_dir + "/" + local_dir)
    
    print("Start generating CNN")
    cnn_orig, rand_im, rand_label = train_cnn2D()
    print("Finish Generating CNN")

    #########################
    #### Set experiment CFG.
    #########################

    
    input_range          = (-mnx.large, mnx.large)
    sample_output_range  = (-5        , mnx.large)
    general_output_range = (-mnx.large, mnx.large)
    out_prop = lambda graph: {n : sample_output_range if n in sample else general_output_range for n in graph.out_l.values()} #TODO
    in_prop  = lambda graph: {n : input_range for n in graph.in_l.values()} #TODO

    ##############################
    ####Measure Verification Step
    ##############################

    coi_measure = dict()
    orig_measure = dict()

    N = 1 #Number of samples per k.
    ks = [1,2,3,4]
    num_out = len(set(cnn_orig.out_l.values()))
    for k in ks : # number of output neurons.
        
        samples = [random.sample(set(cnn_orig.out_l.values()), k=k) for i in range(N)]
        coi_measure[k] = list()
        for j, sample in enumerate(samples):
        
            print("*********************************** COI **********************************************")    
            print("COI nodes:{}".format(str(sample)))
            print("Start solving COI CNN")
            for v in sample:
                if v not in cnn_orig.nodes():
                    raise Exception("{} not in cnn_orig".format(v))
                coi_cnn = mcnn.Cnn2D.coi(cnn_orig, sample)
                start = time.time()        
                coi_cnn.solve(in_prop(coi_cnn), out_prop(coi_cnn))
                end = time.time()
                cor = [max([int(c) for c in re.findall("%(\d+)", samp)]) for samp in sample]
                coi_measure[k].append({"cor"  : cor,
                                       "cor_var"  : None if k == 1 else stat.variance(cor),
                                       "cor_mean" : None if k == 1 else stat.mean(cor),
                                       "nodes"    : len(coi_cnn.nodes()),
                                       "edges"    : len(coi_cnn.edges()),
                                       "time"     : end - start})
                    
                print("Finish solving COI CNN")

    print("*********************************** Original **********************************************")
    print("Start solving CNN")
    start = time.time()        
    #cnn_orig.solve(in_prop(cnn_orig), out_prop(cnn_orig))
    end = time.time()        
    orig_measure = {"cor"  : cor,
                    "cor_var"  : None,
                    "cor_mean" : None,
                    "nodes"    : len(cnn_orig.nodes()),
                    "edges"    : len(cnn_orig.edges()),
                    "time"     : end - start}
    print("Finish solving CNN")

    ####################
    ####Process Results
    ####################
            
    #if len(orig_runtime) == 1:
    #    orig_runtime = orig_runtime * N
    #if len(orig_nodes) == 1:
    #    orig_nodes = orig_nodes * N
    #if len(orig_edges) == 1:
    #    orig_edges = orig_edges * N

    coi_nodes_mean     = {k : stat.mean([samp["nodes"] for samp in measure])     for k,measure in coi_measure.items()}
    coi_nodes_variance = {k : stat.variance([samp["nodes"] for samp in measure]) for k,measure in coi_measure.items()}
    coi_edges_mean     = {k : stat.mean([samp["edges"] for samp in measure])     for k,measure in coi_measure.items()}
    coi_edges_variance = {k : stat.variance([samp["edges"] for samp in measure]) for k,measure in coi_measure.items()}
    coi_rt_mean        = {k : stat.mean([samp["time"] for samp in measure])      for k,measure in coi_measure.items()}
    coi_rt_variance    = {k : stat.variance([samp["time"] for samp in measure])  for k,measure in coi_measure.items()}

    ####################
    ####Plot Graphs
    ####################

    ###NEW GRAPHS
    plt.figure(1)
    plt.plot([n / num_out for n in (list(coi_nodes_mean.keys()) + [num_out])], list(coi_nodes_mean.values()) + [orig_measure["nodes"]],   'r', label='Nodes', markevery=None, marker='o')
    plt.plot([n / num_out for n in (list(coi_edges_mean.keys()) + [num_out])], list(coi_edges_mean.values()) + [orig_measure["edges"]],   'b', label='Edges', markevery=None, marker='o')
    plt.plot([n / num_out for n in (list(coi_rt_mean.keys())    + [num_out])], list(coi_rt_mean.values())    + [orig_measure["time"]],    'g', label='Runtime', markevery=None, marker='o')
    plt.title('Network prop. vs. COI trimming (ratio)')
    plt.legend()
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_prop_vs_trim.png')

    for k in [k for k in coi_measure.keys() if k > 1]:
        cor_var = [samp["cor_var"] for samp in coi_measure[k]]
        nodes   = [samp["nodes"]   for samp in coi_measure[k]]
        plt.figure(2)
        plt.plot(cor_var, nodes, label="k={}".format(k), markevery=None, marker='o', linestyle="None")
    plt.title('Network nodes vs. output nodes variance')
    plt.legend()
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_nodes_vs_variance.png')

    for k in [k for k in coi_measure.keys() if k > 1]:
        cor_var = [samp["cor_var"] for samp in coi_measure[k]]
        edges   = [samp["edges"]   for samp in coi_measure[k]]
        plt.figure(2)
        plt.plot(cor_var, edges, label="k={}".format(k), markevery=None, marker='o', linestyle="None")
    plt.title('Network edges vs. output variance')
    plt.legend()
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_edges_vs_variance.png')    

    for k in [k for k in coi_measure.keys() if k > 1]:
        cor_var = [samp["cor_var"] for samp in coi_measure[k]]
        time   = [samp["time"]   for samp in coi_measure[k]]
        plt.figure(3)
        plt.plot(cor_var, time, label="k={}".format(k), markevery=None, marker='o', linestyle="None")
    plt.title('Network runtime vs. output variance')
    plt.legend()
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_runtime_vs_variance.png')

    exit() #TODO

    ###OLD GRAPHS
    plt.figure(1)
    plt.subplot(131)
    plt.plot(coi_runtime[1], 'rs', orig_runtime, 'b^')
    plt.legend(('COI', 'Orig'))
    plt.title('Runtime [seconds]')
    plt.grid(True)
    plt.subplot(132)
    plt.plot(coi_nodes[1], 'rs', orig_nodes, 'b^')
    plt.legend(('COI', 'Orig'))    
    plt.title('Nodes')
    plt.grid(True)    
    plt.subplot(133)    
    plt.plot(coi_edges[1], 'rs', orig_edges, 'b^')
    plt.legend(('COI', 'Orig'))    
    plt.title('Edges')
    plt.grid(True)
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_coi_vs_orig_1.png')

    plt.figure(2)
    plt.plot(edges_ratio[1], 'b', label='Edges')     
    plt.plot(nodes_ratio[1], 'y', label='Nodes')    
    plt.plot(time_ratio[1], 'g', label='Runtime')
    plt.title('COI / Orig CNN ratio')
    plt.legend()
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_coi_vs_orig_2.png')    

    plt.figure(3)
    plt.plot(time_ratio[1], nodes_ratio[1], 'r', label='Nodes/Time')
    plt.plot(time_ratio[1], edges_ratio[1], 'b', label='Edges/Time')
    plt.title('COI / Orig CNN ratio correlation')
    plt.legend()
    plt.show(block=True) #TODO
    plt.savefig('test_cnn2D_coi_vs_orig_3.png')

    with open('test_cnn2D_output.txt', 'w') as file:
        file.write("COI runtime (seconds):{} \n".format(str(coi_runtime)))
        file.write("COI number of nodes:{} \n".format(coi_nodes))
        file.write("COI number of edges:{} \n".format(coi_edges))
        file.write("Orig runtime (seconds):{} \n".format(str(orig_runtime)))
        file.write("Orig number of nodes:{} \n".format(orig_nodes))
        file.write("Orig number of edges:{} \n".format(orig_edges))                
    os.chdir(mycwd)
    


    

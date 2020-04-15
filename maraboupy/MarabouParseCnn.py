

from maraboupy import Marabou
from maraboupy import MarabouCore
import networkx as nx
#import torch
#from torch_geometric.utils.convert import to_networkx
#from torch_geometric.data import Data

import copy
import matplotlib.pyplot as plt
from math import ceil
from maraboupy import MarabouNetworkNX as mnx

import numpy as np
from tensorflow.python.framework import tensor_util
from tensorflow.python.framework import graph_util
import os
os.environ['TF_CPP_MIN_LOG_LEVEL']='2'
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"
import tensorflow as tf
from tensorflow.python.framework.graph_util import convert_variables_to_constants
from tensorflow.keras import backend
import itertools
#from collections import deque



def l2str(layer_i):
    N = 26
    if layer_i == 0:
        return "a"
    out_str = ""
    while layer_i > 0:
        out_str = chr((layer_i % N) + ord('a')) + out_str
        return out_str
    
def n2str(layer_i,node_i):
    return l2str(layer_i) + str(node_i)

class Filter1D:    
    def __init__(self, weights):
        self.window_size = len(weights)
        self.weights = weights

class Cnn(nx.DiGraph):

    red = '#ff293e'
    blue = '#1f78b4'
    #-------------Graphics: print the network-------------#
    
    def draw_layer(self,layer):      
        self.layout.update({n : ((i * self.draw_scale), - self.next_lyr_pos* self.draw_scale) for i,n in enumerate(layer)})
        self.next_lyr_pos += self.draw_scale

    def cnn_layout(self):
        return self.layout
    
    def print_to_file(self,file, mark=[]):
        fog_x_w = 0.7 * len(self.in_l)
        fig_y_w = 0.7 * max(self.l_num, ceil( 1 * self.l_num))#(2/3)
        plt.figure(figsize=(fog_x_w, fig_y_w), dpi=100)
        labels = {n : n for n in self.nodes()}
        if mark:
            node_colors = [Cnn.red if n in mark else Cnn.blue for n in self.nodes()]
        else :
            node_colors = Cnn.blue       
        nx.draw_networkx(self, pos=self.cnn_layout(), with_labels=True, labels=labels, node_color=node_colors)
        plt.savefig(file)    

    def __str__(self):
        out = ""
        for u,v,w in self.edges.data('weight'):
            out = out + "({},{}), w:={}".format(u,v,w)+ "\n"
        return out

    #-------------Add filters and layers-------------#

    def add_filter(self, f):
        new_l = list()
        self.l_num += 1
        for i in range(len(self.out_l) - f.window_size + 1):
            act_n = n2str(self.l_num, i)
            self.add_node(act_n)
            new_l.append(act_n) 
            for j, w in enumerate(f.weights):
                self.add_edge(self.out_l[i + j], act_n, weight=w)
        self.out_l = new_l
        self.draw_layer(new_l)

    def add_layer(self, w_dict): #Assume some element will be added. in the form of int->int dict.
        self.l_num += 1
        new_l = []
        max_node_new_l = -1
        new_l = list()
        for i in range(len(self.out_l)):
            new_wn = w_dict[i]
            if len(new_wn) == 0:
                continue
            max_n_ind = max(nw[0] for nw in new_wn)
            if max_node_new_l < max_n_ind:
                last_max = max_node_new_l
                max_node_new_l = max_n_ind
                for j in range(last_max + 1, max_node_new_l + 1):
                    self.add_node(n2str(self.l_num, j))
                    new_l.append(n2str(self.l_num, j))
            i_str = n2str(self.l_num - 1, i)
            #print("For i={} we have list={}".format(i,new_wn))
            for wn in new_wn:
                n_str = n2str(self.l_num, wn[0])
                w = wn[1]
                self.add_edge(i_str, n_str, weight=w)
        self.out_l = new_l
        self.draw_layer(new_l)

    #-------------Solve the network-------------#        
        
    def solve(self, in_prop, out_prop):
        #example:
        #in_prop = {n : (-mnx.large, mnx.large) for n in cnn.in_l}
        #out_prop = {n : (-mnx.large, mnx.large) for n in cnn.out_l}
        iq = mnx.networkxToInputQuery(self, in_prop, out_prop)

        print("Start solving")
        vars1, stats1 = MarabouCore.solve(iq, Marabou.createOptions())
        print("Finish solving")
        
        if len(vars1)>0:
            print("SAT")
            print(vars1)
        else:
            print("UNSAT")
                
    #-------------Init-------------#            

    def __init__(self, in_l_size):
        super().__init__()
        self.out_l = list() # L is for layer.        
        self.in_l = list()  # L is for layer.
        self.l_num = 0  # L is for layer.
        self.next_lyr_pos = 0
        self.layout = dict()
        self.draw_scale = 1
        for i in range(in_l_size):
            new_n = n2str(0,i)
            self.add_node(new_n)
            self.out_l.append(new_n)
            self.in_l.append(new_n)
        self.draw_layer(self.in_l)

    #-------------Init from existing NX-------------# 

    def init_from_nx_DiGraph(di_graph):
        cnn = Cnn(0)
        cnn.clear()
        cnn.add_nodes_from(di_graph.nodes())
        cnn.add_edges_from(di_graph.edges())
        cnn.in_l = set() 
        cnn.out_l = set()       
        for n in cnn.nodes():
            if not cnn.in_edges(n):
                cnn.in_l.add(n)
            if not cnn.out_edges(n):
                cnn.out_l.add(n)                
        cnn.l_num = 0  # L is for layer. TODO
        cnn.next_lyr_pos = 0
        cnn.layout = dict()
        visited = set()
        to_visit = cnn.in_l
        while to_visit:
            cnn.draw_layer(to_visit)
            cnn.l_num += 1
            visited = set.union(visited, to_visit)
            to_visit = set.difference(set([s for s in [u for u in to_visit]]), visited)
        return cnn   

    #-------------Find the Cone of Influence of a set of vertices-------------#

    def coi(graph, vertices): # Cone of Influencers
        ancestors = set()
        descendants = set()
        for vertex in vertices:
            ancestors = set.union(ancestors, nx.algorithms.dag.ancestors(graph, vertex)) 
            descendants = set.union(descendants, nx.algorithms.dag.descendants(graph, vertex))
        graph_copy = copy.deepcopy(graph)
        for u in graph:
                if (u not in ancestors) and (u not in vertices) and (u not in descendants):
                    graph_copy.remove_node(u)
        return graph_copy

    #-------------TF to NetworkX-------------#
    
    def get_session(filename):
        with tf.compat.v1.gfile.GFile(filename, "rb") as f:
            graph_def = tf.compat.v1.GraphDef()
            graph_def.ParseFromString(f.read())
        with tf.Graph().as_default() as graph:
            tf.import_graph_def(graph_def, name="")
        return  tf.compat.v1.Session(graph=graph)

    def get_operations(sess):
        return sess.graph.get_operations()
        
    #-------------Keras to TF-------------#
    #https://www.dlology.com/blog/how-to-convert-trained-keras-model-to-tensorflow-and-make-prediction/
    def freeze_session(session, keep_var_names=None, output_names=None, clear_devices=True):
        graph = session.graph
        with graph.as_default():
            freeze_var_names = list(set(v.op.name for v in tf.compat.v1.global_variables()).difference(keep_var_names or []))
            output_names = output_names or []
            output_names += [v.op.name for v in tf.compat.v1.global_variables()]
            # Graph -> GraphDef ProtoBuf
            input_graph_def = graph.as_graph_def()
            if clear_devices:
                for node in input_graph_def.node:
                    node.device = ""
            frozen_graph = convert_variables_to_constants(session, input_graph_def, output_names, freeze_var_names)
        return frozen_graph
    #Use:
    #frozen_graph = freeze_session(backend.get_session(), output_names=[out.op.name for out in model.outputs]) 

    def save_frozen_graph(file_name, frozen_graph):
        # Save to ./model/tf_model.pb
        tf.train.write_graph(frozen_graph, "model", file_name, as_text=False)

    def save_keras_model_to_pb(model, file_name):
        frozen_graph = Cnn.freeze_session(backend.get_session(), output_names=[out.op.name for out in model.outputs])
        Cnn.save_frozen_graph(file_name, frozen_graph)

###############################################################################################################
###############################################################################################################
###############################################################################################################
        
def n2str_md(layer_i,node_cor):
    return l2str(layer_i) + str(node_cor)

class Filter:    
    def __init__(self, weights, bias=None, function="Relu", shape=None):                    
        if function is "Relu":
            if isinstance(weights, list):
                print("This filter is compiled:")
                [print(w) for w in weights]                
                weights = np.array(weights[0].numpy())
                bias = np.array(weights[1])
            self.dim = dict()
            self.dim["x"] = weights.shape[0]
            self.dim["y"] = weights.shape[1]
            self.dim["d"] = weights.shape[2] #Input depth
            self.dim["f"] = weights.shape[3] #Filter amount
            self.weights = weights
            self.bias = bias
        elif function is "MaxPool":
            self.dim = dict()
            self.dim["x"] = shape[0]
            self.dim["y"] = shape[1]
            self.dim["d"] = 1
            self.dim["f"] = None
            self.weights = None
            self.bias = None
        self.function = function

class Cnn2D(nx.DiGraph):
    
    #-------------Init-------------#            

    def __init__(self, in_l_size):
        super().__init__()
        self.in_l = dict()  # L is for layer.
        self.l_num = 0  # L is for layer.
        self.next_lyr_pos = 0
        self.layout = dict()
        self.out_dim = {"x":in_l_size["x"] , "y":in_l_size["y"], "d":in_l_size["d"]}
        self.out_l = dict()
        for x in range(self.out_dim["x"]):
            for y in range(self.out_dim["y"]):
                for d in range(self.out_dim["d"]):
                    new_n = n2str_md(0,[x, y, d])
                    self.add_node(new_n, function="Input")
                    self.out_l[(x,y,d)] = new_n
                    self.in_l[(x,y,d)] = new_n

    def remove_node(self, n):
        keys_out = [k for k,v in self.out_l.items() if v == n]
        for k in keys_out:
            del self.out_l[k]        
        keys_in = [k for k,v in self.in_l.items() if v == n]
        for k in keys_in:
            del self.in_l[k]
        super().remove_node(n)

    def __str__(self):
        out = ""
        for u,v,w in self.edges.data('weight'):
            out = out + "({},{}), w:={}".format(u,v,w)+ "\n"
        return out

    #-------------Add filters and layers-------------#

    def add_filter(self, f):
        #print("init Out dim:" + str([k + "=" + str(v) for k,v in self.out_dim.items()]))
        new_l = dict()        
        self.l_num += 1 #TODO adapt to max pooling
        if f.function is "MaxPool":
            stride = {"x":f.dim["x"], "y":f.dim["y"]}
        else:
            stride = {"x":1, "y":1}
        x_list = range(0, self.out_dim["x"] - f.dim["x"] + 1, stride["x"])
        y_list = range(0, self.out_dim["y"] - f.dim["y"] + 1, stride["y"])
        for x_i, x_cor in enumerate(x_list):
            for y_i, y_cor in enumerate(y_list):
                if f.function is "Relu":
                    f.dim["d"] = self.out_dim["d"]
                    #if f.dim["d"] is not self.out_dim["d"]:
                    #    raise Exception("Relu: Filter and layer depth should be equal")
                    for f_i in range(f.dim["f"]):
                        act_n = n2str_md(self.l_num, [x_i, y_i, f_i])
                        if any([dim > 1 for dim in f.bias[f_i].shape]):
                            raise Exception("Bias should be a scalar. Bias =\n{}\n".format(f.bias[f_i]))
                        self.add_node(act_n, function=f.function, bias=f.bias[f_i])
                        new_l[(x_i, y_i, f_i)] = act_n 
                        for x_j, y_j, d_j in itertools.product(range(f.dim["x"]),range(f.dim["y"]),range(f.dim["d"])):
                            self.add_edge(self.out_l[(x_cor + x_j, y_cor + y_j, d_j)], act_n, weight=f.weights[x_j][y_j][d_j][f_i])
                elif f.function is "MaxPool":
                    f.dim["f"] = self.out_dim["d"]
                    #if f.dim["f"] is not self.out_dim["d"]:
                    #    raise Exception("MaxPool: Filter number and layer depth should be equal")
                    for f_i in range(f.dim["f"]):
                        act_n = n2str_md(self.l_num, [x_i, y_i, f_i])
                        self.add_node(act_n, function=f.function)
                        new_l[(x_i, y_i, f_i)] = act_n
                        for x_j, y_j in itertools.product(range(f.dim["x"]),range(f.dim["y"])):
                            self.add_edge(self.out_l[(x_cor + x_j, y_cor + y_j, f_i)], act_n, weight=1)
        self.out_dim["x"] = len(x_list)
        self.out_dim["y"] = len(y_list)
        self.out_dim["d"] = f.dim["f"]
        self.out_l = new_l
        #print("Post Out dim:" + str([k + "=" + str(v) for k,v in self.out_dim.items()]))

    def add_flatten(self):
        new_l = dict()        
        self.l_num += 1
        for x_i in range(self.out_dim["x"]):
            for y_i in range(self.out_dim["y"]):
                for d_i in range(self.out_dim["d"]):
                    flat_i = (x_i * self.out_dim["y"] + y_i) * self.out_dim["d"] + d_i
                    if flat_i >= self.out_dim["x"]*self.out_dim["y"]*self.out_dim["d"]:
                        raise Exception("Flatten out of size:x={},y={},d={}".format(x_i,y_i,d_i))
                    act_n = n2str_md(self.l_num, [flat_i,0,0])
                    self.add_node(act_n, function="Flatten")
                    new_l[(flat_i,0,0)] = act_n
                    self.add_edge(self.out_l[(x_i,y_i,d_i)], act_n, weight=1)
        self.out_l = new_l
        self.out_dim = {"x": len(new_l), "y":1, "d":1}
        
    def add_dense(self, weights, function="Relu"): #Assume some element will be added. in the form of cor->(cor,weight) dict.
        
        weights_mat = weights[0].numpy()
        bias_vec = weights[1].numpy()
        print("This is bias=" + str(bias_vec))
        if sum([1 for dim in bias_vec.shape if dim > 1]) > 1 or len(bias_vec.shape) > 2:
            raise Exception("Bias should be a 1d vector. Bias vec=\n{}\n".format(bias_vec))
        w_dict = {(i,0,0):[((j,0,0), weights_mat[i][j]) for j in range(weights_mat.shape[1])] for i in range(weights_mat.shape[0])} #i is source, j is target.
        
        self.l_num += 1
        new_l = dict()
        max_t_x = 0
        max_t_y = 0
        max_t_d = 0
        for x,y,d in self.out_l:
            #print("x={},y={},d={}".format(x,y,d))
            if (x,y,d) not in w_dict:
                print(self.out_l.keys())
                print(w_dict.keys())
                raise Exception("Node not in w_dict, x={},y={},d={}\n w_dict=".format(x,y,d,str(w_dict)))                
            cor_w = w_dict[(x,y,d)]
            if len(cor_w) == 0:
                continue
            source = n2str_md(self.l_num - 1, [x,y,d])
            for cor, w in cor_w:
                target = n2str_md(self.l_num, list(cor))
                if cor not in new_l:
                    self.add_node(target, bias=bias_vec[cor[0]] ,function=function)
                    new_l[tuple(cor)] = target
                    max_t_x = max(cor[0], max_t_x)
                    max_t_y = max(cor[1], max_t_y)
                    max_t_d = max(cor[2], max_t_d)
                self.add_edge(source, target, weight=w)
        #print("Finished stage 1")
        for x,y,d in itertools.product(range(max_t_x+1),range(max_t_y+1),range(max_t_d+1)):
            if (x,y,d) not in new_l:
                target = n2str_md(self.l_num, [x,y,d])
                self.add_node(target, function="Relu")
                new_l[(x,y,d)] = target
        #print("Finished stage 2")                
        self.out_l = new_l
        self.out_dim = {"x":max_t_x+1 , "y":max_t_y+1 , "d":max_t_d+1 }

    #-------------Solve the network-------------#        
        
    def solve(self, in_prop, out_prop):
        #example:
        #in_prop = {n : (-mnx.large, mnx.large) for n in cnn.in_l}
        #out_prop = {n : (-mnx.large, mnx.large) for n in cnn.out_l}
        iq = mnx.networkxToInputQuery(self, in_prop, out_prop)

        print("Start solving")
        vars1, stats1 = MarabouCore.solve(iq, Marabou.createOptions())
        print("Finish solving")
        
        if len(vars1)>0:
            print("SAT")
            print(vars1)
        else:
            print("UNSAT")

        
                
    #-------------Find the Cone of Influence of a set of vertices-------------#

    def coi(graph, vertices): # Cone of Influencers
        ancestors = set()
        descendants = set()
        for vertex in vertices:
            ancestors = set.union(ancestors, nx.algorithms.dag.ancestors(graph, vertex)) 
            descendants = set.union(descendants, nx.algorithms.dag.descendants(graph, vertex))
        remove_nodes = [u for u in graph.nodes() if (u not in ancestors) and (u not in vertices) and (u not in descendants)]
        graph_copy = copy.deepcopy(graph)
        for u in remove_nodes:
            graph_copy.remove_node(u)
        return graph_copy


    def keras_model_to_Cnn2D(in_l_size, model):
        cnn = Cnn2D(in_l_size)
        for layer in model.layers:
            if layer.get_config()["name"].startswith("conv2d"):
                f = Filter(layer.weights)
                cnn.add_filter(f)                
            elif layer.get_config()["name"].startswith("max_pooling2d"):
                f = Filter([],function="MaxPool", shape=list(layer.get_config()["pool_size"]))
                cnn.add_filter(f)
            elif layer.get_config()["name"].startswith("flatten"):
                cnn.add_flatten()
            elif layer.get_config()["name"].startswith("dense"):
                print("In shape:{}\nOut shape:{}".format(layer.input_shape, layer.output_shape))
                print(layer.weights)
                weights = layer.weights[0].numpy()
                #cnn.add_dense({(i,0,0):[((j,0,0), weights[i][j]) for j in range(layer.output_shape[1])] for i in range(layer.input_shape[1])}) #TODO no bias included
                cnn.add_dense(layer.weights)
                
        return cnn

        

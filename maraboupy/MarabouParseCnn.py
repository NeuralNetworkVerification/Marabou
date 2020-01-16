

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

class Filter:    
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
        
    def solve(in_prop, out_props):
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
        
'''   

    def my_node_eq(lhs, rhs, session):
        if lhs.shape.as_list() != rhs.shape.as_list():
            print("shape false")
            return False
        else :
            print("same shape, result={}".format(lhs == rhs))
            
            return all(tf.math.equal(lhs,rhs).eval(session=session))
        
    #-------------TF to NetworkX-------------#
    #https://gist.github.com/Tal-Golan/94d968001b5065260e5dae4774bc6f7a#file-build_networkx_subgraph_from_tf_graph-py
    def tf_graph_to_nx(startingPoints,endPoints, session):

        def childNodes(curNode):
            isGradientNode=lambda node: node.name.split(sep='/')[0]=='gradients'            
            return set([childNode.experimental_ref() for op in curNode.deref().consumers() for childNode in op.outputs if not isGradientNode(childNode)])
          
        # visit all nodes in the graph between startingPoints (deep net input) and endPoints (predictions)
        # and build a networkx directed graph        
        DG=nx.DiGraph()
        nodesToVisit=deque([s.experimental_ref() for s in startingPoints])
        nodesVisited=deque()
        while nodesToVisit:
            curNode=nodesToVisit.popleft()
            nodesVisited.append(curNode)
            DG.add_node(curNode)
            node_eq_list = [Cnn.my_node_eq(endPoint,curNode.deref(), session) for endPoint in endPoints]
            print("List: {}".format(node_eq_list))
            if not any(node_eq_list):
                nodesToVisit.extend(childNodes(curNode)-set(nodesVisited)-set(nodesToVisit))
                for childNode in childNodes(curNode):
                    DG.add_edge(curNode, childNode)

        print("Found these nodes: {}".format(len(DG.nodes())))
        print("Found these edges: {}".format(len(DG.edges())))
        
        
        # make sure its acyclic
        assert nx.is_directed_acyclic_graph(DG), "loops detected in the graph"
        return DG


    #-------------Keras to NetworkX-------------#
    def keras_to_nx(model):
        session = tf.compat.v1.keras.backend.get_session()
        #frozen_graph = Cnn.freeze_session(tf.compat.v1.keras.backend.get_session(), output_names=[out.op.name for out in model.outputs])
        inputs  = model.inputs
        outputs = model.outputs
        print("Found these inputs: {}".format(inputs))
        print("Found these outputs: {}".format(outputs))        
        return Cnn.tf_graph_to_nx(inputs, outputs, session)'''
                
            



from maraboupy import Marabou
import networkx as nx
import queue
import copy
import matplotlib.pyplot as plt

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

    def __init__(self, in_l_size):
        super().__init__()
        self.out_l = [] # L is for layer.        
        self.in_l = []  # L is for layer.
        self.l_num = 0  # L is for layer.        
        for i in range(in_l_size):
            new_n = n2str(0,i)
            self.add_node(new_n)
            self.out_l.append(new_n)
            self.in_l.append(new_n)

    def __str__(self):
        out = ""
        for e in self.edges():
            out = out + str(e) + "\n"
        return out

    def add_filter(self, f):
        new_l = []
        self.l_num += 1
        for i in range(len(self.out_l) - f.window_size + 1):
            act_n = n2str(self.l_num, i)
            self.add_node(act_n)
            new_l.append(act_n) 
            for j, w in enumerate(f.weights):
                self.add_edge(self.out_l[i + j], act_n, weight=w)
        self.out_l = new_l

    def add_layer(self, w_dict): #Assume some element will be added. in the form of int->int dict.
        self.l_num += 1
        new_l = []
        max_node_new_l = 0
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
            i_str = n2str(self.l_num - 1, i)
            print("For i={} we have list={}".format(i,new_wn))
            for wn in new_wn:
                n_str = n2str(self.l_num, wn[0])
                w = wn[1]
                self.add_edge(i_str, n_str, weight=w)                                                                       
    
    
def cone_of_influencers(graph, vertex):
    graph.remove_nodes_from(nx.algorithms.dag.descendants(graph,vertex))
    ancestors = nx.algorithms.dag.ancestors(graph,vertex)
    #print(ancestors)
    graph_copy = copy.deepcopy(graph)
    for u in graph:
        if (u not in ancestors) and (u != vertex):
            graph_copy.remove_node(u)
    return graph_copy
 
if __name__ == "__main__":

    cnn = Cnn(6)
    f = Filter([1, 1, 1])
    cnn.add_filter(f)
    cnn.add_filter(f)
    W = {
        0 : [(0,1),(1,1),(2,1)],
        1 : [(0,1),(1,1),(2,1)]
        }
    cnn.add_layer(W)
    print(cnn)

    labels = {}
    for n in cnn.nodes():
        labels[n] = n
    nx.draw_networkx(cnn, pos=nx.circular_layout(cnn))
    plt.savefig('testplot.png')
    
    '''for n in cnn.out_l:
        print("COI {}\n".format(n))
        print(cone_of_influencers(cnn,n))'''

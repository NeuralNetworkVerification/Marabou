from maraboupy import MarabouNetworkNX as mnx
from maraboupy import MarabouParseCnn as mcnn
from itertools import combinations, product
from math  import ceil
import numpy as np
import matplotlib.pyplot as plt
import random


def create_cnn2D(in_l_len, f_w, layers):
    cnn2d = mcnn.Cnn2D(in_l_len)
    f = mcnn.Filter(f_w)
    for j in range(layers):
        cnn2d.add_filter(f)
    return cnn2d

def avg_coi_nodes(cnn, subset_size):
    avg = lambda list: 0 if len(list) == 0 else (sum(list) / len(list))
    return avg([mcnn.Cnn.coi(cnn, list(elements)).number_of_nodes() for elements in combinations(cnn.out_l, subset_size)])

def measure_coi_size(in_l_len, f_len, layers, subset_sizes):
    cnn = create_cnn(in_l_len, [1 for i in range(f_len)], layers)
    return cnn, cnn.number_of_nodes(), {subset_size : avg_coi_nodes(cnn,subset_size) for subset_size in subset_sizes}        
        
if __name__ == "__main__": 
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
    print("Out dim:" + str([k + "=" + str(v) for k,v in cnn.out_dim.items()]))
    
    for n,v in cnn.nodes.items():
        print(str(n) + ":" + (v["function"] if "function" in v else ""))
    for i,e in enumerate(sorted(cnn.edges, key= lambda e : e[::-1])):
        print(str(i) + ":" + str(e) + ":" + str(cnn.edges[e]["weight"]))

    in_prop  = {n : (-mnx.large, mnx.large) for n in cnn.in_l.values()}
    out_prop = {n : (-mnx.large, mnx.large) for n in cnn.out_l.values()}
    cnn.solve(in_prop, out_prop)
        

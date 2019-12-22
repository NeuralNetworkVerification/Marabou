from maraboupy import MarabouNetworkNX as mnx
from maraboupy import MarabouParseCnn as mcnn
from itertools import combinations, product
from math  import ceil
import matplotlib.pyplot as plt
import random


def create_cnn(in_l_len, f_w, layers):
    cnn = mcnn.Cnn(in_l_len)
    f = mcnn.Filter(f_w)
    for j in range(layers):
        cnn.add_filter(f)
    return cnn

def avg_coi_nodes(cnn, subset_size):
    avg = lambda list: 0 if len(list) == 0 else (sum(list) / len(list))
    return avg([mcnn.Cnn.coi(cnn, list(elements)).number_of_nodes() for elements in combinations(cnn.out_l, subset_size)])

def measure_coi_size(in_l_len, f_len, layers, subset_sizes):
    cnn = create_cnn(in_l_len, [1 for i in range(f_len)], layers)
    return cnn, cnn.number_of_nodes(), {subset_size : avg_coi_nodes(cnn,subset_size) for subset_size in subset_sizes}        
        
if __name__ == "__main__":

    subset_sizes = range(1,5)
    frac_of_nodes = {s : [] for s in subset_sizes}
    filter_sizes = range(3,10)
    in_l_len = [60]
    layers = [6]
    for in_l, f_len, lrs in product(in_l_len, filter_sizes, layers):
        cnn, n_nodes, ss_sizes_dict = measure_coi_size(in_l, f_len, lrs, subset_sizes)
        [frac_of_nodes[n].append((ss_sizes_dict[n]) / n_nodes) for n in subset_sizes]
        print("Done with filters of size={}".format(f_len))
        cnn.print_to_file("cnn_visual_{}_{}_{}.png".format(in_l,f_len,lrs), mcnn.Cnn.coi(cnn,random.sample(cnn.out_l, 3)))


    plt.figure()
    plt.xlabel('Filter Size')
    plt.ylabel('Fraction of nodes in COI')
    for ss_size in subset_sizes:
        line, = plt.plot(filter_sizes, frac_of_nodes[ss_size])        
        line.set_label("COI of {} output nodes".format(ss_size))
    plt.legend()
    plt.savefig("measure_avg_coi_{}_{}_{}.png".format(in_l_len[-1],filter_sizes[-1],layers[-1]))
    print("done")
    

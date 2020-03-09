from maraboupy import Marabou, MarabouCore
import networkx as nx

large = 10000.0
verbose_g = False

def my_print(s):
    global verbose_g
    if verbose_g:
        print(s)

def networkxToInputQuery(net, input_bounds, output_bounds, verbose=False):
    global verbose_g
    verbose_g = verbose 
    var_offset = lambda u: 0 if net.nodes[u]["function"] is not "Relu" or u in input_nodes else 1
    degree = {n : {'in':0,'out':0} for n in net.nodes()}
    incoming = {n : [] for n in net.nodes()}
    for u, nbrdict in  net.adjacency():
        for v, wdict in nbrdict.items():
            degree[u]['out'] += 1
            degree[v]['in'] += 1
            incoming[v].append((u,wdict["weight"]))

    my_print("Incoming=")
    for k,v in incoming.items():
        my_print(str(k) + ":" + str(v))

    input_nodes = set()
    output_nodes = set()
    var_list = list()
    for n in net.nodes():
        in_deg = degree[n]['in']
        out_deg = degree[n]['out']
        var_list.append(n)
        if in_deg == 0 and out_deg > 0:
            input_nodes.add(n)
        elif in_deg > 0 and net.nodes[n]["function"] is "Relu":
            var_list.append(n)
        if out_deg == 0:
            output_nodes.add(n)

    my_print("Incoming=")
    for k,v in incoming.items():
        my_print(str(k) + ":" + str(v))

    my_print("var_list=")
    for i,v in enumerate(var_list):
        my_print("{}:{}".format(str(i),str(v)))

    my_print("input_nodes={}".format(str([v for v in input_nodes])))
    my_print("output_nodes={}".format(str([v for v in output_nodes])))    

    inputQuery = MarabouCore.InputQuery()
    inputQuery.setNumberOfVariables(len(var_list))

    for n in input_nodes:
        inputQuery.setLowerBound(var_list.index(n), input_bounds[n][0])
        inputQuery.setUpperBound(var_list.index(n), input_bounds[n][1])

    for n in output_nodes:        
        inputQuery.setLowerBound(var_list.index(n) + var_offset(n), output_bounds[n][0] if net.nodes[n]["function"] is not "Relu" else 0) 
        inputQuery.setUpperBound(var_list.index(n) + var_offset(n), output_bounds[n][1])
        my_print("Set bound v{} in [{},{}] (output)".format(var_list.index(n) + var_offset(n), output_bounds[n][0] if net.nodes[n]["function"] is not "Relu" else 0, output_bounds[n][1]))
        
    for n in filter(lambda n: incoming[n] and net.nodes[n]["function"] in {"Relu", "Flatten"}, net.nodes()):        
        equation = MarabouCore.Equation()
        equation.addAddend(-1, var_list.index(n))
        [[equation.addAddend(w, var_list.index(u) + var_offset(u))] for u,w in incoming[n]]
        my_print("v{} = sum{}".format(var_list.index(n), str(["{}*v{}".format(w,var_list.index(u) + var_offset(u)) for u,w in incoming[n]])))
        equation.setScalar(0)
        inputQuery.addEquation(equation)

    for i,v in enumerate(var_list):
        if v is not var_list[i]:
            raise Exception("WTF")
        if i == 0:
            continue
        if v == var_list[i - 1]:
            if v not in output_nodes:
                inputQuery.setLowerBound(i, 0)
                inputQuery.setUpperBound(i, large)
                my_print("Set bound v{} in [{},{}] (Relu)".format(i, 0, large))
            my_print("Set v{}=ReLu(v{})".format(i, i-1))
            MarabouCore.addReluConstraint(inputQuery, i-1, i)

    for n in filter(lambda n: incoming[n] and net.nodes[n]["function"] is "MaxPool", net.nodes()):
        my_print("Set MaxConstraint {}=max({})".format(var_list.index(n), set([var_list.index(u[0]) for u in incoming[n]]))) 
        MarabouCore.addMaxConstraint(inputQuery, set([var_list.index(u[0]) for u in incoming[n]]), var_list.index(n))

    return inputQuery

    

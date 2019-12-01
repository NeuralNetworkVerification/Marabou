from maraboupy import Marabou, MarabouCore
import networkx as nx

large = 10.0

def networkxToInputQuery(net, input_bounds, output_bounds):

    degree = {n : {'in':0,'out':0} for n in net.nodes()}
    incoming = {n : [] for n in net.nodes()}
    for u, nbrdict in  net.adjacency():
        for v, wdict in nbrdict.items():
            degree[u]['out'] += 1
            degree[v]['in'] += 1
            #degree[u] = (degree[u]['in'], degree[u]['out'] + 1)
            #degree[v] = (degree[v]['in'] + 1, degree[v]['out'])
            incoming[v].append((u,wdict["weight"]))

    input_nodes = set()
    output_nodes = set()
    var_list = list()
    for n in net.nodes():
        in_deg = degree[n]['in']
        out_deg = degree[n]['out']
        var_list.append(n)
        if in_deg == 0 and out_deg > 0:
            input_nodes.add(n)
        elif in_deg > 0 and out_deg > 0:
            var_list.append(n)
        elif in_deg > 0 and out_deg == 0:
            output_nodes.add(n)
                                                        
    inputQuery = MarabouCore.InputQuery()
    inputQuery.setNumberOfVariables(len(var_list))

    for n in input_nodes:
        inputQuery.setLowerBound(var_list.index(n), input_bounds[n][0])
        inputQuery.setUpperBound(var_list.index(n), input_bounds[n][1])

    for n in output_nodes:
        inputQuery.setLowerBound(var_list.index(n), output_bounds[n][0]) 
        inputQuery.setUpperBound(var_list.index(n), output_bounds[n][1])

    for n in filter(lambda n: incoming[n], net.nodes()):
        equation = MarabouCore.Equation()
        equation.addAddend(-1, var_list.index(n))
        [[equation.addAddend(w, var_list.index(u)) if u in input_nodes else equation.addAddend(w, var_list.index(u) + 1)] for u,w in incoming[n]]
        equation.setScalar(0)
        inputQuery.addEquation(equation)

    for i,v in enumerate(var_list,1):
        if i >= len(var_list):
            break
        if v == var_list[i - 1]:
            inputQuery.setLowerBound(i, 0)
            inputQuery.setUpperBound(i, large)
            MarabouCore.addReluConstraint(inputQuery,i-1,i)
            
    return inputQuery

    

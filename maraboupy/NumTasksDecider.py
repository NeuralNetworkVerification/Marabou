'''
/* *******************                                                        */
/*! \file NumTasksDecider
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Christopher Lazarus
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Methods that split the input regions
 **
 ** [[ Add lengthier description here ]]
 **/
'''

import numpy as np
import math
import copy


def split_input_regions(ranges, input_name, k, strategy, pb_name):
    """
    Split the input region into 2^k subregions

    Args:
        ranges      ((np.array, np.array)) Lowerbounds and upperbounds of input variables
        input_name  (str) Name of the input variable in the .pb representation of the network
        k           (int) The number of time to bisect the input region
        strategy    (int) Heuristic used to decide the influence of an input interval
        pb_name     (str) Name of the .pb file representing the network

    Return:
        A list of triples. Each triple has the form {region-id, lowerbounds, upperbounds}
    """
    p_id = 0
    ranges = [ranges]
    for i in np.arange(k):
        new_ranges = []
        for range in ranges: # For each sub range, split further
            cur_lbs = range[0]
            cur_ubs = range[1]
            if strategy == 1:
                next_dim = boundaryPatternVariance(cur_lbs, cur_ubs,
                                                   input_name, pb_name)
            if strategy == 2:
                next_dim = largestInterval(cur_lbs, cur_ubs)
            if strategy == 3:
                next_dim = boundaryPatternDifference(cur_lbs, cur_ubs,
                                                     input_name, pb_name)
            r1, r2 = bisect_dimension(cur_lbs, cur_ubs, next_dim)
            new_ranges.append(r1)
            new_ranges.append(r2)
        ranges = new_ranges
    pids = list(map(str, np.arange(2**k)))
    return list(zip(pids, ranges))



# Strategy 1
def boundaryPatternVariance(inputMins, inputMaxs, input_name, name=None, seed=0):
    """
    For each input interval, sample relu patterns at its lowerbounds and upperbounds,
    return the dimension that has the highest value of
    |variance(relu_l ; relu_u) / (variance(relu_l) + variance(relu_u))|
    """
    import tensorflow as tf
    if name:
        with tf.compat.v1.gfile.FastGFile(name + ".pb", "rb") as f:
            graph_def = tf.compat.v1.GraphDef()
            graph_def.ParseFromString(f.read())

        with tf.Graph().as_default() as graph:
            tf.import_graph_def(graph_def, name="")

        dists = np.zeros(len(inputMins))
        for dim in range(len(inputMins)):
            if inputMins[dim] == inputMaxs[dim]:
                continue
            np.random.seed(seed)
            points = np.transpose([np.random.uniform(low=inputMins[x], high=inputMaxs[x],
                                   size=10) for x in range(len(inputMaxs))])
            points_l = points.copy()
            points_l[:, dim] = inputMins[dim]
            points_u = points.copy()
            points_u[:, dim] = inputMaxs[dim]
            points_all = np.concatenate((points_l, points_u), axis=0)


            tensor_in = graph.get_tensor_by_name(input_name)
            relus_ops = [x for x in graph.get_operations() if x.type=="Relu"]
            out_relus = tf.concat([relu.outputs[0] for relu in relus_ops], 1)
            with tf.compat.v1.Session(graph=graph) as sess:
                relu_values = np.array(sess.run(out_relus, feed_dict={
                    tensor_in: points_all
                }))
            shape0 = relu_values.shape[0]
            shape1 = relu_values.shape[1]
            relu_signature = np.zeros([shape0, shape1], dtype=np.int)
            for i in range(shape0):
                relu_signature[i] = (np.append([], relu_values[i,:]) > 0)
            pat_l = relu_signature[: int(shape0 / 2)]
            pat_u = relu_signature[int(shape0 / 2):]
            pat_all = relu_signature
            v1 = np.var(pat_l, dtype=np.float64, axis=0).sum()
            v2 = np.var(pat_u, dtype=np.float64, axis=0).sum()
            v3 = np.var(pat_all, dtype=np.float64, axis=0).sum()
            if v3 == 0:
                dists[dim] = 0
            elif v1 + v2 == 0:
                return dim
            else:
                dists[dim] = v3 / (v1 + v2)
    return np.argmax(dists)

# Strategy 2
def largestInterval(lowerbounds, upperbounds):
    """
    Return the dimension with the largest absolute interval
    """
    return np.argmax(np.subtract(upperbounds, lowerbounds))

# Strategy 3
def boundaryPatternDifference(inputMins, inputMaxs, input_name,
                              name=None, parts=4,  num_samples=100, seed=0):
    """
    Return the dimension that causes the greatest variation of relu patterns
    """
    import tensorflow as tf
    if name:
        with tf.compat.v1.gfile.FastGFile(name + ".pb", "rb") as f:
            graph_def = tf.compat.v1.GraphDef()
            graph_def.ParseFromString(f.read())

        with tf.Graph().as_default() as graph:
            tf.import_graph_def(graph_def, name="")

        dists = np.zeros(len(inputMins))
        for dim in range(len(inputMins)):
            if inputMins[dim] == inputMaxs[dim]:
                continue
            np.random.seed(seed)

            points = np.transpose([np.random.uniform(low=inputMins[x], high=inputMaxs[x],
                                   size=num_samples) for x in range(len(inputMaxs))])

            interval = (inputMaxs[dim] - inputMins[dim]) / parts
            points_all = []
            for i in range(parts + 1):
                points_l = points.copy()
                if i == parts:
                    points_l[:, dim] = inputMaxs[dim]
                else:
                    points_l[:, dim] = inputMins[dim] + i * interval
                points_all.append(points_l)
            points_all = np.concatenate(points_all)

            tensor_in = graph.get_tensor_by_name(input_name)
            relus_ops = [x for x in graph.get_operations() if x.type=="Relu"]
            out_relus = tf.concat([relu.outputs[0] for relu in relus_ops], 1)
            with tf.compat.v1.Session(graph=graph) as sess:
                relu_values = np.array(sess.run(out_relus, feed_dict={
                    tensor_in: points_all
                }))
            shape0 = relu_values.shape[0]
            shape1 = relu_values.shape[1]
            relu_signature = np.zeros([shape0, shape1], dtype=np.int)
            for i in range(shape0):
                relu_signature[i] = (np.append([], relu_values[i,:]) > 0)
            diffs = []
            for i in range(parts):
                pat_l = relu_signature[i * num_samples : (i + 1) * num_samples]
                pat_u = relu_signature[(i + 1) * num_samples: (i + 2) * num_samples]
                diffs.append(np.linalg.norm((pat_l - pat_u), ord=1, axis=1))
            dists[dim] = np.array(diffs).sum()

    return np.argmax(dists)

def bisect_dimension(lowerbounds, upperbounds, dim):
    """
    Returns two ranges of input variables with the dim-th original interval bisected
    """
    mid = (lowerbounds[dim] + upperbounds[dim]) / 2
    lb1 = lowerbounds.copy()
    ub1 = upperbounds.copy()
    ub1[dim] = mid
    lb2 = lowerbounds.copy()
    lb2[dim] = mid
    ub2 = upperbounds.copy()
    return (lb1, ub1), (lb2, ub2)

'''
/* *******************                                                        */
/*! \file DnCParallelSolver.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Methods related to the parallel execution of the divide-and-conquer solving
 **
 ** [[ Add lengthier description here ]]
 **/
'''


from maraboupy.AcasNet import AcasNet
from maraboupy import MarabouUtils
from maraboupy import NumTasksDecider

import numpy as np

import copy, random, time, os
import multiprocessing as mp
from multiprocessing import Process, Pipe, Lock, Value
from multiprocessing.connection import wait
import subprocess

def solve_subproblems(network_name, subproblems, property_path, runtimes,
                      tree_states, init_to=0, num_workers=16,
                      online_split=1, to_factor=1.5, strategy=3,
                      input_name="input:0", seed=4, log_file=None):
    """
    Solve the given set of subqueries on the network using divide-and-conquer algorithm

    Args:
        network_name    (str) Name of the network
        subproblems     (list of pairs) A list of {query-timeout} pairs
        property_path   (str) Path to the property file
        runtimes        (dict) Dictionary that maps sub-query's id to its runtime
        tree_states     (dict) Dictionary that maps sub-query's id to its tree states
        init_to         (int) Initial timeout
        num_workers     (int) Number of worker threads to create
        online_split    (int) Number of times to bisect a subquery when timeout occurs
        to_factor       (float) The timeout factor
        strategy        (int) Heuristic used to decide the influence of an input interval
        input_name      (str) Name of the input variable in the .pb representation of the network
        seed            (int) Random seed
        log_file        (str) If not None, log the stats of each subproblem while solving

    Return:
        Empty list if UNSAT
        A list containing a satisfying assignment if SAT
    """
    inputs = []
    for p in subproblems:
        inputs.append((p, init_to))
    random.Random(seed).shuffle(inputs)

    # Create LIFO Queue for each worker
    L = []
    for i in range(num_workers):
        L.append(mp.Manager().list())
    for i, input_ in enumerate(inputs):
        L[i % num_workers].append(input_)

    # Create and initiate workers
    num_tasks = Value("i", len(subproblems))
    processes = []
    readers = []
    return_lst = []
    lock = Lock()
    for i in range(num_workers):
        r, w = Pipe(duplex=True)
        readers.append(r)
        p = mp.Process(target=worker,args=(network_name, property_path, L, num_tasks,
                                           online_split, to_factor, log_file, w, lock,
                                           i, strategy, input_name))
        processes.append(p)
        p.daemon = True
        p.start()
        w.close()

    # Wait for results
    while readers:
        for r in wait(readers):
            try:
                val_, results = r.recv()
                return_lst.append(results)
                if len(val_) > 0:
                    for results in return_lst:
                        for result in results:
                            val, t_id, runtime, has_TO, num_tree_states = result
                            if has_TO:
                                runtimes[t_id] = TO
                            else:
                                runtimes[t_id] = runtime
                            tree_states[t_id] = num_tree_states
                    with num_tasks.get_lock():
                        num_tasks.value = 0
                    for p in processes:
                        while p.is_alive():
                            subprocess.run(["kill", "-9",  str(p.pid)])
                    return val_
            except EOFError:
                readers.remove(r)

    for p in processes:
        p.join()

    for results in return_lst:
        for result in results:
            val, t_id, runtime, has_TO, num_tree_states = result
            if has_TO:
                runtimes[t_id] = -runtime
            else:
                runtimes[t_id] = runtime
            tree_states[t_id] = num_tree_states
    return []



def worker(network_name, property_path, L, num_tasks, online_split, to_factor,
           log_file, w, lock, thread_id, strategy=3, input_name="input:0"):
    """
    Worker that obtains a LIFO queue and tries to solve all queries in it
    using the tree-search strategy.
    Send solving statistics to the main thread using pipe

    Args:
        network_name    (str) Name of the network
        property_path   (str) Path to the property file
        L               (list mp.Manager.List) List of LIFO work queues
        num_tasks       (mp.Value) Remaining number of queries to solve
        online_split    (int) Number of times to bisect a subquery when timeout occurs
        to_factor       (float) The timeout factor
        log_file        (str) if not None, log the stats of each subproblem while solving
        w               (Connection) Pipe connection object for sending stats to the main thread
        lock            (mp.Lock) Lock object to prevent race conditions
        thead_id        (int) id of the current thread
        strategy        (int) Heuristic used to decide the influence of an input interval
        input_name      (str) Name of the input variable in the .pb representation of the network
    """
    results = []
    val = []
    factor = 1
    l = L[thread_id]
    np.random.seed(thread_id)
    num_workers = len(L)
    while num_tasks.value != 0:
        try:
            with lock:
                input_ = l.pop()
            query = input_[0]
            TO = input_[1]

            # Solve the subquery and log results
            result = encode(network_name + ".nnet", property_path, query, TO)
            val, t_id, runtime, has_TO, num_tree_states = result
            logResult(log_file, result, thread_id, num_tasks.value)


            if has_TO:
                # If timeout occured, split the query into more subqueries
                try:
                    subproblems = NumTasksDecider.split_input_regions(query[1],
                                                                      input_name,
                                                                      online_split,
                                                                      strategy,
                                                                      network_name)
                except:
                    if log_file:
                        with open(log_file, "a") as out_file:
                            out_file.write("Failed to create subproblems for "
                                           "{}!".format(t_id))
                num_created = 2 ** online_split
                assert(num_created == len(subproblems))
                with lock:
                    with num_tasks.get_lock():
                        num_tasks.value += num_created
                selectedLs = np.random.choice(range(num_workers), size=num_created, replace=True)
                for i in range(num_created):
                    p = subproblems[i]
                    p_ = (query[0] + "-" + p[0], p[1])
                    with lock:
                        L[selectedLs[i]].append((p_, int(TO * to_factor)))
                with num_tasks.get_lock():
                    num_tasks.value -= 1
            elif len(val) > 0:
                # If SAT, quit the loop
                break
            else:
                # If UNSAT, continue
                with num_tasks.get_lock():
                    num_tasks.value -= 1
            results.append(result)
        except:
            pass
        if num_workers > 1:
            with lock:
                size = len(l)
                if random.randint(0, size) == size:
                    # Choose victim
                    victim = L[np.random.choice(range(num_workers))]
                    balance_qs(l, victim, 2)
    w.send((val, results))
    w.close()
    return


def encode(nnet_file_name, property_path, trial, TO):
    """
    Solving a query

    Args:
        nnet_file_name    (str) Path to the .nnet representation of the network
        property_path     (str) Path to the property file
        trial             (triple) (region_id, lowerbounds, upperbounds)
        TO                (int) Timeout for solving this query

    Return:
        list:     A satisfying assignment (empty if UNSAT or TIMEOUT)
        str:      Region id
        float:    Runtime
        bool:     whether timeout occured
        int:      Number of tree states
    """
    t_id = trial[0]
    input_mins = trial[1][0]
    input_maxs = trial[1][1]
    net = AcasNet(nnet_file_name, property_path, input_mins, input_maxs)
    try:
        assignment, stats = net.solve(timeout=TO)
        has_TO = stats.hasTimedOut()
        if has_TO:
            num_tree_states = 0
        else:
            num_tree_states = stats.getNumVisitedTreeStates()
        return assignment, t_id, stats.getTotalTime()/1000., has_TO, num_tree_states
    except:
        return [], "error-" + t_id, TO, True, 0

def balance_qs(l1, l2, threshold):
    """
    Balance two LIFO queues such that the difference of their sizes is within a threshold
    """
    s1 = len(l1)
    s2 = len(l2)
    diff = s1 - s2
    if abs(diff) < threshold:
        return
    elif diff > 0:
        maxl = l1
        minl = l2
    else:
        maxl = l2
        minl = l1
    while (len(minl) < len(maxl)):
        minl.append(maxl.pop())
    return


def logResult(log_file, result, thread_id, num_tasks):
    """
    Write the statistics for solving a sub-query into log_file
    """
    if log_file:
        val, t_id, runtime, has_TO, num_tree_states = result
        with open(log_file, "a") as out_file:
            out_file.write("Thread {}, ".format(thread_id))
            out_file.write("{} remaining, ".format(num_tasks))
            out_file.write(str(t_id) + ", " + str(runtime) + ", " +
                            str(num_tree_states) + ", ")
            if has_TO:
                out_file.write( "TO\n")
            elif len(val) > 0:
                out_file.write(" SAT\n")
            else:
                out_file.write(" UNSAT\n")



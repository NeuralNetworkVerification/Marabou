'''
/* *******************                                                        */
/*! \file DnCSolver.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The class that performs divide-and-conquer solving
 **
 ** [[ Add lengthier description here ]]
 **/
'''

from maraboupy.AcasNet import AcasNet
from maraboupy import DnCParallelSolver as DnC
from maraboupy import NumTasksDecider

import numpy as np
import random, time

class DnCSolver:
    """
    Class representing Divide and Conquer Solver
    """
    def __init__(self, network_name, property_path, num_workers=1,
                 initial_splits=1, online_split=1, init_to=10,
                 to_factor=1.5, dim_heuristic=3, input_name="input:0",
                 seed=4, log_file=None):
        """
        Constructor of the DnCSolver.
        Args:
            network_name     (str) Name of the network (e.g. ACASXU_run2a_1_1_batch_2000)
            property_path    (str) Name of the property file
            num_workers      (int) Number of worker threads to create
            initial_splits   (int) Number times to bisect the input region initially
            online_split     (int) Number of times to bisect a subquery when timeout occurs
            to_sub           (int) Initial timeout given to the subquery
            to_factor        (float) Timeout factor
            dim_heuristic    (int) Heuristic used to decide the influence of an input interval
                                    1: largest variance in relu patterns on boundaries
                                    2: largest absolute interval
                                    3: largest variance in relu patterns
            input_name       (str) Name of the input variable in the .pb representation of the network
            seed             (int) Random seed
            log_file         (str) If provided, log the solving statistics for each subproblems
        """
        self.start_time = time.time()

        # Network related
        self.input_name=input_name
        self.network_name = network_name

        # Query related
        self.property_path = property_path
        self.get_input_ranges()
        self.num_inputs = len(self.input_mins)

        # Solving related
        self.initial_splits = initial_splits
        self.num_workers = num_workers
        self.property_path = property_path
        self.online_split = online_split
        self.strategy = dim_heuristic
        self.init_to = init_to
        self.to_factor = to_factor


        # Statistics and results
        self.assignment = None
        self.runtimes = None
        self.SAT = False
        self.error = False
        self.tree_states = None
        self.total_runtime = 0

        # Other
        self.seed = seed
        self.log_file = log_file
        self.init_log(log_file)

    def solve(self, subproblems):
        """
        Solve the given list of subproblems using the divide-and-conquer algorithm

        Args:
            subproblems     (list of triples) Subproblems to be solved
        """
        if not self.SAT:
            self.runtimes = {}
            self.tree_states = {}
            self.assignment = DnC.solve_subproblems(self.network_name, subproblems,
                                                    self.property_path, self.runtimes,
                                                    self.tree_states, self.init_to,
                                                    self.num_workers, self.online_split,
                                                    self.to_factor, self.strategy,
                                                    self.input_name, self.seed,
                                                    self.log_file)
            self.SAT = len(self.assignment) > 0
            self.total_runtime = time.time() - self.start_time
        self.log_final_stats()
        if self.SAT:
            print ("SAT")
        else:
            print ("UNSAT")
        return



    def get_input_ranges(self):
        """
        Obtain the input region of the query
        """
        net = AcasNet(self.network_name + ".nnet", self.property_path)
        self.input_mins, self.input_maxs = net.getInputRanges()

    def initial_split(self):
        """
        Initial division of the input query
        """
        ranges = (self.input_mins, self.input_maxs)
        return NumTasksDecider.split_input_regions(ranges, self.input_name,
                                                   self.initial_splits,
                                                   self.strategy,
                                                   self.network_name)

    """
    Below are methods for writing to the log and summary file
    """
    def init_log(self, log_file):
        if log_file:
            try:
                with open(log_file, "a") as out_file:
                    print("Valid log file!")
                    out_file.write("Network name: " + self.network_name + "\n")
                    out_file.write("Property file: " + self.property_path + "\n")
                    out_file.write("Number of threads: " +  str(self.num_workers) + " \n")
                    out_file.write("Initial splits: {}\n".format(self.initial_splits))
                    out_file.write("Online split: {}\n".format(self.online_split))
                    out_file.write("Initial timeout: {}\n".format(self.init_to))
                    out_file.write("Timeout factor: {}\n".format(self.to_factor))
                    out_file.write("Dimension strategy {}\n".format(self.strategy))
            except:
                print("Invalid log file!\n")


    def log_final_stats(self):
        if self.log_file:
            with open(self.log_file, "a") as out_file:
                out_file.write("\nSolving finished!\n")


                out_file.write("Total time: %f\n" %(self.total_runtime))
                if self.runtimes is not None:
                    solve_time = 0
                    effective_time = 0
                    for key in self.runtimes:
                        t = self.runtimes[key]
                        if t >= 0:
                            solve_time += t
                            effective_time += t
                        else:
                            solve_time -= t
                    out_file.write("Solve time: %f\n" %(solve_time))
                    out_file.write("Effective time: %f\n" %(effective_time))
                    out_file.write("Number of sub-tasks: {}\n".format(len(self.runtimes)))
                if self.tree_states is not None:
                    s = 0
                    for key in self.tree_states:
                        s += self.tree_states[key]
                    out_file.write("Total visited tree states (solve) %f\n" %(s))
                if self.SAT:
                    out_file.write("SAT\n")
                    for line in self.assignment:
                        out_file.write(line + "\n")
                else:
                    out_file.write("UNSAT\n")

    def write_summary_file(self, summaryFile, TO):
        try:
            with open(summaryFile, 'w') as outFile:
                if TO:
                    outFile.write("TIMEOUT ")
                elif self.error:
                    outFile.write("ERROR ")
                elif self.SAT:
                    outFile.write("SAT ")
                else:
                    outFile.write("UNSAT ")
                outFile.write("%.2f " %(self.total_runtime))
                t = 0
                if (not self.SAT) and self.tree_states is not None:
                    for key in self.tree_states:
                        t += self.tree_states[key]
                    outFile.write("%d " %(int(t)))
                else:
                    outFile.write("0 ")
                outFile.write("0\n")
        except:
            print ("Fail to write summary file!")

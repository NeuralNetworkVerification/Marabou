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
 ** \brief Main method that calls the divide-and-conquer solver
 **
 ** [[ Add lengthier description here ]]
 **/
'''

"""

"""
from maraboupy import DnCSolver
from maraboupy import Options

import numpy as np
from multiprocessing import Process, Pipe
import os

def main():
    """
    The main method
    Checking a property (property.txt) on a network (network.nnet) and
      save the results to a summary file (summary.txt)
      "python3 DnC.py -n network -q property.txt --summary-file=summary.txt"

    Call "python3 DnC.py --help" to see other options
    """

    options, args = Options.create_parser().parse_args()

    try:
        property_path = options.query
    except:
        assert(os.path.isfile(property_path))
        exit()
    try:
        network_name = options.network_name
        assert(os.path.isfile(network_name + ".pb"))
        assert(os.path.isfile(network_name + ".nnet"))
    except:
        print("Fail to import network!")
        exit()


    # Get arguments from input
    (num_workers, input_name,
    initial_splits, online_split,
    init_to, to_factor, strategy,
    seed, log_file, summary_file) = Options.get_constructor_arguments(options)

    solver = DnCSolver.DnCSolver(network_name, property_path, num_workers,
                                 initial_splits, online_split, init_to, to_factor,
                                 strategy, input_name, seed, log_file)
    solver.write_summary_file(summary_file, True)

    try:
        # Initial split of the input region
        parent_conn, child_conn = Pipe()
        p = Process(target=getSubProblems, args=(solver, child_conn))
        p.start()
        sub_queries = parent_conn.recv()
        p.join()

        # Solve the created subqueries
        solver.solve(sub_queries)
    except KeyboardInterrupt:
        solver.write_summary_file(summary_file, True)
    else:
        solver.write_summary_file(summary_file, False)
    return

def getSubProblems(solver, conn):
    sub_queries = solver.initial_split()
    conn.send(sub_queries)
    conn.close()


if __name__ == "__main__":
    main()

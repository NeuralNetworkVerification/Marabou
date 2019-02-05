'''
/* *******************                                                        */
/*! \file Options.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Methods to parse options to DnC.py
 **
 ** [[ Add lengthier description here ]]
 **/
'''



from optparse import OptionParser


def str_to_lst(lst_str):
    if lst_str:
        return list(map(int, lst_str[1:-1].split(",")))
    return None

def get_constructor_arguments(options):
    return (options.num_workers, options.input_name, options.initial_splits,
            options.online_split, options.to_sub, options.to_factor,
            options.strategy, options.seed, options.log_file, options.summary_file)

def create_parser():
    parser = OptionParser(usage="usage: %prog [options] formula outfile",
                      version="%prog 1.0")
    parser.add_option("-q", "--query",
                      dest="query",
                      type="str",
                      default=None,
                      help="Required. Path to the acas property file")
    parser.add_option("-n", "--net",
                      dest="network_name",
                      type="str",
                      default=None,
                      help="Required. Path to the network")
    parser.add_option("-w", "--workers",
                      dest="num_workers",
                      type="int",
                      default=4,
                      help="Number of workers/cpus, default: 1")
    parser.add_option("--input-name",
                      dest="input_name",
                      type="str",
                      default="input:0",
                      help="Name of the input operation in the network file")
    parser.add_option("--initial-splits",
                      dest="initial_splits",
                      type="int",
                      default=0,
                      help="The number of times to bisect the problem initially")
    parser.add_option("--online-split",
                      dest="online_split",
                      type="int",
                      default= 2,
                      help="The number of times to bisect the problem"
                           "when time out is reached")
    parser.add_option("--to-sub",
                      dest="to_sub",
                      type=int,
                      default=5,
                      help="Timeout for solving the original problem")
    parser.add_option("--to-factor",
                      dest="to_factor",
                      type=float,
                      default=1.5,
                      help="Time out factor")
    parser.add_option("--strategy",
                      dest="strategy",
                      type=int,
                      default=3,
                      help="Strategy to pick the bisecting dimension")
    parser.add_option("-s", "--seed",
                      dest="seed",
                      type=int,
                      default=4,
                      help="Random seed")
    parser.add_option("--log-file",
                      dest="log_file",
                      type="str",
                      default=None,
                      help="The file where the logs are stored.")
    parser.add_option("--summary-file",
                      dest="summary_file",
                      type="str",
                      default=None,
                      help="Summary file for experiment.")
    return parser

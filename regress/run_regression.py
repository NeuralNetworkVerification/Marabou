#!/usr/bin/env python3

import os
import subprocess
import sys
import threading
from timeit import default_timer as timer
import json
import argparse


DEFAULT_TIMEOUT = 600


def run_process(args, cwd, timeout, s_input=None):
    """Runs a process with a timeout `timeout` in seconds. `args` are the
    arguments to execute, `cwd` is the working directory and `s_input` is the
    input to be sent to the process over stdin. Returns the output, the error
    output and the exit code of the process. If the process times out, the
    output and the error output are empty and the exit code is 124."""

    proc = subprocess.Popen(
        args,
        cwd=cwd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)

    out = ''
    err = ''
    exit_status = 124
    try:
        if timeout:
            timer = threading.Timer(timeout, lambda p: p.kill(), [proc])
            timer.start()
        out, err = proc.communicate(input=s_input)
        exit_status = proc.returncode
    finally:
        if timeout:
            timer.cancel()

    if isinstance(out, bytes):
        out = out.decode()
    if isinstance(err, bytes):
        err = err.decode()
    return (out.strip(), err.strip(), exit_status)


def run_marabou(marabou_binary, network_path, property_path, expected_result, arguments=None):
    '''
    Run marabou and assert the result is according to the expected_result
    :param marabou_binary: path to marabou executable
    :param network_path: path to nnet file to pass too marabou
    :param property_path: path to property file to pass to marabou to verify
    :param expected_result: SAT / UNSAT
    :param arguments list of arguments to pass to Marabou (for example DnC mode)
    :return: True / False if test pass or not
    '''
    if not os.access(marabou_binary, os.X_OK):
        sys.exit(
            '"{}" does not exist or is not executable'.format(marabou_binary))
    if not os.path.isfile(network_path):
        sys.exit('"{}" does not exist or is not a file'.format(network_path))
    if not os.path.isfile(network_path):
        sys.exit('"{}" does not exist or is not a file'.format(property_path))
    if expected_result not in {'SAT', 'UNSAT'}:
        sys.exit('"{}" is not a marabou supported result'.format(expected_result))

    args = [marabou_binary, network_path, property_path]
    if isinstance(arguments, list):
        args += arguments
    out, err, exit_status = run_process(args, os.curdir, DEFAULT_TIMEOUT)

    if exit_status != 0:
        print("exit status: {}".format(exit_status))
        return False
    if err != '':
        print("err: {}".format(err))
        return False

    # If the output is UNSAT there is no \n after the UNSAT statement
    if expected_result == 'UNSAT':
        out_lines = out.splitlines()
        return out_lines[-1] == expected_result
    else:
        return '\nSAT\n' in out


def run_folder_on_property(folder, property_file):
    results = {}
    for net in os.listdir(folder):
        start = timer()
        result = run_marabou("build/Marabou", os.path.join(folder, net), property_file, "SAT")
        end = timer()
        results[net] = (end - start, result)
        print("{}, time(sec): {}, SAT: {}".format(net, end - start, result))

    return results


def main():
    parser = argparse.ArgumentParser(
        description=
        'Runs benchmark and checks for correct exit status and output.')

    parser.add_argument('marabou_binary')
    parser.add_argument('network_file')
    parser.add_argument('property_file')
    parser.add_argument('expected_result', choices=('SAT', 'UNSAT'))
    parser.add_argument('--dnc', action='store_true')
    args = parser.parse_args()
    marabou_binary = os.path.abspath(args.marabou_binary)
    network_file = os.path.abspath(args.network_file)
    property_file = os.path.abspath(args.property_file)
    expected_result = args.expected_result

    marabou_args = []
    if args.dnc:
        marabou_args += ['--dnc']
    # print(marabou_binary)
    # print(network_file)
    # print(property_file)
    # print(expected_result)
    # timeout = DEFAULT_TIMEOUT

    return run_marabou(marabou_binary, network_file, property_file, expected_result, marabou_args)

# ./build/Marabou resources/nnet/coav/reluBenchmark0.041867017746s_UNSAT.nnet resources/propertie
# s/builtin_property.txt

if __name__ == "__main__":
    if main():
        sys.exit(0)
    else:
        sys.exit(1)

    # coav_results = run_folder_on_property("resources/nnet/coav/", 'resources/properties/builtin_property.txt')
    # twin_results = run_folder_on_property("resources/nnet/twin/", 'resources/properties/builtin_property.txt')
    #
    # json.dump(coav_results, open("coav_results.json", "w"))
    # json.dump(twin_results, open("twin_results.json", "w"))

    # print(run_marabou("build/Marabou", "resources/nnet/acasxu/ACASXU_experimental_v2a_1_9.nnet",
    #             "resources/properties/builtin_property.txt", "SAT"))

    print(run_marabou("build/Marabou", "resources/nnet/twin/twin_ladder-5_inp-4_layers-5_width-10_margin.nnet",
                      "resources/properties/builtin_property.txt", "UNSAT"))
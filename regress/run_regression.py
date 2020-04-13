import argparse
import os
import subprocess
import sys
import threading

DEFAULT_TIMEOUT = 600
EXPECTED_RESULT_OPTIONS = ('sat', 'unsat')


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


def analyze_process_result(out, err, exit_status, expected_result):
    if exit_status != 0:
        print("exit status: {}".format(exit_status))
        return False
    if err != '':
        print("err: {}".format(err))
        return False

    # If the output is unsat there is no \n after the unsat statement
    if expected_result == 'unsat':
        out_lines = out.splitlines()
        if out_lines[-1] == expected_result:
            return True
        else:
            print("Expected unsat, but last line is in output is: ", out_lines[-1])
            return False
    else:
        if '\nsat' in out:
            return True
        else:
            print('expected sat, but \'\\nSAT\' is not in the output. tail of the output:\n', out[-1500:])
            return False


def run_marabou(marabou_binary, network_path, property_path, expected_result, timeout=DEFAULT_TIMEOUT, arguments=None):
    '''
    Run marabou and assert the result is according to the expected_result
    :param marabou_binary: path to marabou executable
    :param network_path: path to nnet file to pass too marabou
    :param property_path: path to property file to pass to marabou to verify
    :param expected_result: sat / unsat
    :param arguments list of arguments to pass to Marabou (for example DnC mode)
    :return: True / False if test pass or not
    '''
    if not os.access(marabou_binary, os.X_OK):
        sys.exit(
            '"{}" does not exist or is not executable'.format(marabou_binary))
    if not os.path.isfile(network_path):
        sys.exit('"{}" does not exist or is not a file'.format(network_path))
    if not os.path.isfile(property_path):
        sys.exit('"{}" does not exist or is not a file'.format(property_path))
    if expected_result not in {'sat', 'unsat'}:
        sys.exit('"{}" is not a marabou supported result'.format(expected_result))

    args = [marabou_binary, network_path, property_path]
    if isinstance(arguments, list):
        args += arguments
    out, err, exit_status = run_process(args, os.curdir, timeout)

    return analyze_process_result(out, err, exit_status, expected_result)


def run_mpsparser(mps_binary, network_path, expected_result, arguments=None):
    '''
    Run marabou and assert the result is according to the expected_result
    :param marabou_binary: path to marabou executable
    :param network_path: path to nnet file to pass too marabou
    :param property_path: path to property file to pass to marabou to verify
    :param expected_result: sat / unsat
    :param arguments list of arguments to pass to Marabou (for example DnC mode)
    :return: True / False if test pass or not
    '''
    if not os.access(mps_binary, os.X_OK):
        sys.exit(
            '"{}" does not exist or is not executable'.format(mps_binary))
    if not os.path.isfile(network_path):
        sys.exit('"{}" does not exist or is not a file'.format(network_path))
    if expected_result not in {'sat', 'unsat'}:
        sys.exit('"{}" is not a marabou supported result'.format(expected_result))

    args = [mps_binary, network_path]
    if isinstance(arguments, list):
        args += arguments
    out, err, exit_status = run_process(args, os.curdir, DEFAULT_TIMEOUT)

    return analyze_process_result(out, err, exit_status, expected_result)


def main():
    parser = argparse.ArgumentParser(
        description=
        'Runs benchmark and checks for correct exit status and output. supports nnet and mps file format')

    parser.add_argument('marabou_binary')
    parser.add_argument('network_file')
    parser.add_argument('property_file', nargs='?', default='')
    parser.add_argument('expected_result', choices=EXPECTED_RESULT_OPTIONS)
    parser.add_argument('--dnc', action='store_true')
    parser.add_argument('--timeout', nargs='?', const=DEFAULT_TIMEOUT, type=int)

    args = parser.parse_args()

    binary = args.marabou_binary
    network_file = os.path.abspath(args.network_file)
    expected_result = args.expected_result

    if args.network_file.endswith('nnet'):
        property_file = os.path.abspath(args.property_file)

    marabou_args = []
    if args.dnc:
        marabou_args += ['--dnc']
    if args.network_file.endswith('nnet'):
        return run_marabou(binary, network_file, property_file, expected_result, args.timeout, marabou_args)
    elif args.network_file.endswith('mps'):
        return run_mpsparser(binary, network_file, expected_result, marabou_args)
    else:
        raise NotImplementedError('supporting only nnet and mps file format')


if __name__ == "__main__":
    if main():
        sys.exit(0)
    else:
        sys.exit(1)

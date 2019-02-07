import os, stat
import sys
import tempfile
import zipfile
from argparse import ArgumentParser, ArgumentTypeError
from logging import warning
import shutil
from experiment_structure import *
import re


# <------- defining argument parser -------> #
SHORT_CLUSTER_HOURS = 3


def my_time_type(s):
    m = re.match(r'(?P<hours>^\d+$)|(?P<string>\d+:\d\d:\d\d)', s)
    if not m:
        raise ArgumentTypeError('timout should be specified as num of '
                                'hours or in H:MM:SS format (hours can be more '
                                'than on digit).')
    if m.group('hours'):
        return f'{s}:00:00'
    return s


parser = ArgumentParser(fromfile_prefix_chars='@')
parser.add_argument('name', help='name for the experiment')
parser.add_argument('solvers',
                    help='executable for the experiment. for multiple, '
                         'separate by comma')
parser.add_argument('--flags', help='flags for the executables. if more than one '
                                    'solver is specified, separate by comma '
                                    'respectively for the solver', nargs='?', default='')
nn_group = parser.add_mutually_exclusive_group()
nn_group.add_argument('--nnlist', help='list of networks, separated by a '
                                       'comma')
nn_group.add_argument('--nndir', help='directory of the networks')
parser.add_argument('-f', '--filter',
                    help='get only the networks match the filter (using regex)',
                    nargs='?', default='')
parser.add_argument('property', help='property file to test')
parser.add_argument('timeout', help='timeout for each task', type=my_time_type)
parser.add_argument('qos', help='qos for slurm', nargs='?')
parser.add_argument('cpus', help='number of cpus each task will use',
                    default=1, type=int,
                    nargs='?')


# <------- END defining argument parser ------->


def build_sbatch_file(sbatch_file_dir_path, job_name, identifier, solver,
                      flag, network, property_file, timeout,
                      qos, cpus_per_task):
    # set some variables
    sbatch_file_name = f'{identifier}.sbatch'
    summary_file_name = f'{identifier}.sum'
    job_output_file_name = f'task_{job_name}_%j_{identifier}.out'

    sbatch_file_abs_path = os.path.join(sbatch_file_dir_path, sbatch_file_name)
    summary_file_rel_path = os.path.join(SOLVER_RESULT_PATH, summary_file_name)
    job_output_rel_path = os.path.join(SLURM_RESULT_PATH, job_output_file_name)

    # use the copied files
    solver = os.path.join(SOLVERS_PATH, os.path.basename(solver), 'run.sh')
    network = os.path.join(NNET_PATH, os.path.basename(network))
    property_file = os.path.join(BASE_DIR, os.path.basename(property_file))

    # write the sbatch file
    with open(sbatch_file_abs_path, 'w', newline='\n') as slurm_file:
        slurm_file.write('#!/bin/bash\n')
        slurm_file.write(f'#SBATCH --job-name={job_name}\n'.format(job_name=job_name))
        slurm_file.write(f'#SBATCH --cpus-per-task={cpus_per_task}\n')
        slurm_file.write(f'#SBATCH --output={job_output_rel_path}\n')
        slurm_file.write(f'#SBATCH --qos={qos}\n')
        #slurm_file.write(f'#SBATCH --qos={qos}\n')
        slurm_file.write(f'#SBATCH --time={timeout}\n')
        slurm_file.write(f'#SBATCH --signal=B:SIGUSR1@300\n')
        slurm_file.write(f'\n')

        slurm_file.write(f'\n')
        slurm_file.write(f'function extract_result {{\n')
        slurm_file.write(f'\tsource /barrett/data/zeljic/py3.6/bin/activate\n')
        slurm_file.write(f'\tpython3 {os.path.join(UTILS_PATH, "result_extractor.py")}\n')
        slurm_file.write(f'\tdeactivate\n')
        slurm_file.write(f'}}\n')
        slurm_file.write(f'\n')

        # get process group id
        slurm_file.write(f'pgid=$(($(ps -o pgid= -p $$)))\n')
        # catch signals and pass them to child processes
        slurm_file.write(f'trap \'kill -TERM $pid & wait $pid ; extract_result ; exit\' SIGUSR1\n')
        slurm_file.write(f'\n')

        slurm_file.write(f'pgid=$(($(ps -o pgid= -p $$)))\n')
        slurm_file.write(f'trap \'kill -TERM -$pgid & extract_result\' SIGTERM\n')
        slurm_file.write(f'\n')

        slurm_file.write(f'pwd; hostname; date\n')
        slurm_file.write(f'\n')

        # save current working directory before running script
        slurm_file.write(f'cwd=$(pwd)\n')
        slurm_file.write(f'\n')

        # sbatch the job, with summary file
        # also keep the pid of it
        slurm_file.write(f'{solver} {network} {property_file} '
                         f'{summary_file_rel_path} '
                         f'\'{flag}\' '
                         f'& pid=$!\n')
        # wait for the subprocess
        slurm_file.write(f'wait $pid\n')
        slurm_file.write(f'\n')

        # return to the previously saved working directory
        slurm_file.write(f'cd $cwd\n')
        slurm_file.write(f'\n')

        # after each job, we'll take the result and try (if there will be
        # enough data) to create a graph using all already finished jobs
        # It'll let us be able to see comparisons even if there are undone jobs,
        # or if there was an interruption and some of the jobs won't finish at all
        slurm_file.write(f'extract_result\n')
        slurm_file.write(f'\n')

        slurm_file.write(f'date\n')

    return os.path.basename(slurm_file.name)


def build_run_script(zipfile, shfile_path, sbatch_files, solvers):
    with open(f'{shfile_path}', 'w', newline='\n') as run_file:

        # unzip resources
        run_file.write(f'unzip {zipfile}\n')
        run_file.write(f'\n')

        # give execution permissions
        for solver in solvers:
            solver = os.path.join(SOLVERS_PATH, os.path.basename(solver), 'run.sh')
            run_file.write(f'chmod a+x {solver}\n')

        # sbatch everything
        for file in sbatch_files:
            run_file.write(f'sbatch {os.path.join(SBATCH_FILES_PATH, file)}\n')

    # file with path, to later use
    return run_file.name


def copy_resources(solvers, solvers_dir, networks, networks_dir, property, property_dir):
    for solver in solvers:
        # shutil.copy2(solver, solvers_dir)
        solver_name = os.path.splitext(os.path.basename(solver))[0]
        shutil.unpack_archive(solver, os.path.join(solvers_dir, solver_name))

    for network in networks:
        shutil.copy2(network, networks_dir)

    shutil.copy2(property, property_dir)


def details_file(path, name, solvers, flags, networks, prop, timeout):
    details_file_path = os.path.join(path, EXPERIMENT_DETAILS_FILENAME)
    with open(details_file_path, 'w') as file:
        file.write(f'experiment: {name}\n')

        file.write(f'solvers:\n')
        for s, (solver, flag) in enumerate(zip(solvers, flags)):
            file.write(f'solver_{s}:{solver}{"_" + flag if flag else ""}\n')

        file.write(f'networks:\n')
        for n, nnet in enumerate(networks):
            file.write(f'nnet_{n}:{os.path.basename(nnet)}\n')

        file.write(f'property: {os.path.basename(prop)}\n')
        file.write(f'timeout: {timeout}\n')


def calculate_qos_from_timeout(qos, timeout):
    m = re.match(r'(?P<H>\d+):.*', timeout)
    hours = int(m['H'])
    if qos is None:
        qos = 'max2' if hours <= SHORT_CLUSTER_HOURS else 'max12'
    elif (qos != 'max12' or qos != 'cav24h') and  hours > SHORT_CLUSTER_HOURS:
        qos = 'max12'
        warning(f'given timeout {timeout} not match the '
                f'given qos \'short\' '
                f'qos will be changed to \'{qos}\'')
    return qos


def build(name, solvers, flags, networks, property, timeout, qos, cpus_per_task):
    sbatch_files = []

    with tempfile.TemporaryDirectory() as temp_dir:

        #                                  #
        # <---- create zip structure ----> #
        #                                  #
        # resources dir
        base_dir_abs_path = os.path.join(temp_dir, BASE_DIR)
        os.mkdir(base_dir_abs_path)
        # utils dir
        utils_abs_path = os.path.join(temp_dir, UTILS_PATH)
        os.mkdir(utils_abs_path)
        # solvers dir
        solvers_dir = os.path.join(temp_dir, SOLVERS_PATH)
        os.mkdir(solvers_dir)
        # networks dir
        nnet_dir = os.path.join(temp_dir, NNET_PATH)
        os.mkdir(nnet_dir)
        # tmp dir
        tmp_dir = os.path.join(temp_dir, TMP_PATH)
        os.mkdir(tmp_dir)
        # result dir
        # create also the result dirs, otherwise the solver and slurm won't be able to create output
        result_dir = os.path.join(temp_dir, RESULT_PATH)
        os.mkdir(result_dir)
        # slurm result dir
        os.mkdir(os.path.join(temp_dir, SLURM_RESULT_PATH))
        # solver result dir
        os.mkdir(os.path.join(temp_dir, SOLVER_RESULT_PATH))
        # comparisons dir
        os.mkdir(os.path.join(temp_dir, COMPARISION_PATH))

        #                                            #
        # <---- create experiment details file ----> #
        #                                            #
        solver_names = [os.path.splitext(os.path.basename(n))[0] for n in solvers]
        details_file(base_dir_abs_path, name, solver_names, flags, networks, property, timeout)

        #                        #
        # <---- copy files ----> #
        #                        #
        # copy utils for later use
        shutil.copy2('result_extractor.py', utils_abs_path)
        shutil.copy2('experiment_structure.py', utils_abs_path)
        # copy all experiment resources
        copy_resources(solvers, solvers_dir, networks, nnet_dir, property, base_dir_abs_path)

        #                                     #
        # <---- create the sbatch files ----> #
        #                                     #
        sbatch_dir_abs_path = os.path.join(temp_dir, SBATCH_FILES_PATH)
        os.mkdir(sbatch_dir_abs_path)

        for s, (solver, flag) in enumerate(zip(solver_names, flags)):
            for n, network in enumerate(networks):
                file_identifier = f'solver_{s}_nnet_{n}'
                file = build_sbatch_file(
                    sbatch_dir_abs_path, name, file_identifier,
                    solver, flag, network, property,
                    timeout, qos, cpus_per_task)
                sbatch_files.append(file)

        #                                                #
        # <---- archive everything for portability ----> #
        #                                                #
        # create directory for experiment files
        base_export_dir_name = f'{name}'
        os.mkdir(base_export_dir_name)

        # compress everything
        zipfile = shutil.make_archive(f'{os.path.join(base_export_dir_name, name)}',
                                      'zip', temp_dir)
        zipfile = os.path.relpath(zipfile, base_export_dir_name)

        # create run script
        shfile_path = os.path.join(base_export_dir_name, f'run_{name}.sh')
        run_file = build_run_script(zipfile, shfile_path, sbatch_files, solver_names)
        os.chmod(run_file, stat.S_IRWXU)


def glob_re(pattern, strings):
    try:
        return list(filter(re.compile(pattern).match, strings))
    except re.error:
        warning('invalid regex filter, no filter is applied')
        return strings


def validate_arguments(name, solvers, flags, networks,
                       property, timeout, qos, cpus):
    is_valid = True
    msg = ''

    # validate existence of files
    is_valid, msg = (is_valid and True, msg + '') if all(map(zipfile.is_zipfile, solvers)) \
        else (False , msg + 'solver files dose not exists or are not zip archive\n')
    is_valid, msg = (is_valid and True, msg + '') if all(map(os.path.isfile, networks)) \
        else (False, msg + 'networks files dose not exists\n')
    is_valid, msg = (is_valid and True, msg + '') if os.path.isfile(property) \
        else (False, msg + 'property file dose not exists\n')

    if not is_valid:
        warning(f'invalid arguments:\n{msg}')
        exit(-1)


def main(args):
    # get list of the networks
    if args.nndir is not None:
        # directory of networks is given
        files = [os.path.join(args.nndir, f) for f in os.listdir(args.nndir)]
        if args.filter != '':
            networks = glob_re(args.filter, files)
        else:
            networks = files
    else:
        # list of networks is given
        networks = list(map(str.strip, args.nnlist.split(',')))

    solvers = list(map(str.strip, args.solvers.split(',')))

    if not args.flags:
        args.flags = ',' * (len(solvers) - 1)
    flags = list(map(str.strip, args.flags.split(',')))

    qos = calculate_qos_from_timeout(args.qos, args.timeout)

    validate_arguments(args.name, solvers, flags, networks,
                       args.property, args.timeout, args.qos, args.cpus)

    build(args.name, solvers, flags, networks, args.property,
          args.timeout, qos, args.cpus)


if __name__ == '__main__':
    if len(sys.argv) == 2:
        # act as got file
        args = parser.parse_args([sys.argv[1]])
    else:
        # act as got args
        args = parser.parse_args()

    main(args)

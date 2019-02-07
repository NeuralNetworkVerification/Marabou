from itertools import combinations
from typing import Dict

import numpy as np

from experiment_structure import *
import re
# since cluser server dosen't have an X11 display
import matplotlib

RESULTS_FILE_FORMAT_PATTERN = r'(?P<result>SAT|UNSAT|TIMEOUT|ERROR|UNKNOWN) ' \
                              r'(?P<time_in_sec>\d+(\.\d+)?) ' \
                              r'(?P<visited_states>\d+) ' \
                              r'(?P<avg_ptime_mu>\d+)'

matplotlib.use('Agg')
import matplotlib.pyplot as plt
import tabulate

ResultTable = Dict[str, Dict[str, 'OneExperimentResult']]


def read_experiment_details(path_to_file):
    solvers = {}
    networks = {}
    with open(path_to_file) as file:
        name = re.match(r'experiment: (.*)', file.readline()).group(1)  # experiment name
        file.readline()  # solvers:
        line = file.readline()
        while line.startswith('solver_'):
            m = re.match(r'(?P<index>solver_\d+):(?P<solver>.*)', line)
            solvers[m.group('index')] = m.group('solver')
            line = file.readline()

        # line == 'networks:'
        line = file.readline()
        while line.startswith('nnet_'):
            m = re.match(r'(?P<index>nnet_\d+):(?P<nnet>.*)', line)
            networks[m.group('index')] = m.group('nnet')
            line = file.readline()

        # line == 'property:<file>'
        property = re.match(r'property: (.*)', line).group(1)
        line = file.readline()
        m = re.match(r'timeout:\s*?(?P<H>\d+):(?P<M>\d\d):(?P<S>\d\d)', line)
        timeout = int(m['H']) * 3600 + int(m['M']) * 60 + int(m['S'])

        return SlurmResults(name, solvers, networks, property, timeout)


def bucket_result_files():
    """
    split all the result files into a 2 dim table,
    where the rows are the nnet id, the cols are the solver id,
    and the cell values are the matching file name
    """
    # result file name pattern
    name_pattern = r'(?P<solver>.*?_\d+)_(?P<nnet>.*?_\d+).sum$'
    solver_results_files = os.listdir(os.path.join(BASE_DIR, RESULTS_DIR, SOLVER_RESULT_DIR))

    results_buckets = {}
    for file in solver_results_files:
        m = re.match(name_pattern, file)
        if m:
            solver = m.group('solver')
            nnet = m.group('nnet')
            if nnet not in results_buckets:
                results_buckets[nnet] = {}
            results_buckets[nnet][solver] = file
    return results_buckets


class OneExperimentResult:
    def __init__(self, result, time, visited_states, avg_pivoting_time_mu):
        self.result = result
        self.time = time
        self.visited_states = visited_states
        self.avg_pivoting_time_mu = avg_pivoting_time_mu

    @staticmethod
    def from_file(file_path):
        result_pattern = RESULTS_FILE_FORMAT_PATTERN
        
        with open(file_path) as file:
            line = file.readline()
            m = re.match(result_pattern, line)
            if m is None:
                print(f'Line: {line}\n in file {file_path}\n does not match pattern {result_pattern}')
            result = m.group('result')
            time = float(m.group('time_in_sec'))
            visited_states = int(m.group('visited_states'))
            avg_ptime_mu = float(m.group('avg_ptime_mu'))
            return OneExperimentResult(result, time, visited_states, avg_ptime_mu)

        
    def __repr__(self):
        return f'{self.result} {self.time} {self.visited_states} {self.avg_pivoting_time_mu}'


class SlurmResults:
    def __init__(self, name, solvers, networks, property, timeout):
        self.name: str = name
        self.solvers: Dict[str, str] = solvers
        self.networks: Dict[str, str] = networks
        self.property: str = property
        self.timeout: float = timeout

        self.result_table: ResultTable = None

    @staticmethod
    def from_file(file):
        return read_experiment_details(file)

    def read_results(self, result_files):
        self.result_table = {nnet: {solver: None for solver in self.solvers}
                             for nnet in self.networks}

        for nnet, solvers_result in result_files.items():
            for solver, file in solvers_result.items():
                self.result_table[nnet][solver] = OneExperimentResult.from_file(
                    os.path.join(BASE_DIR, RESULTS_DIR, SOLVER_RESULT_DIR, file))

    def export_time_comparision(self, solver1, solver2):
        assert solver1 in self.solvers
        assert solver2 in self.solvers
        assert self.result_table is not None, 'call read_result first'

        solver_one_values = []
        solver_two_values = []
        for nnet, solvers_result in self.result_table.items():
            if solvers_result.get(solver1, None) and \
                    solvers_result.get(solver2, None):
                if not {solvers_result[solver1].result, solvers_result[solver1].result} & {'ERROR', 'UNKNOWN'}:
                    # we don't want errors here
                    solver_one_values.append(solvers_result[solver1].time)
                    solver_two_values.append(solvers_result[solver2].time)

        if not solver_one_values:
            # no matching results for the same network
            return

        range_end = min(self.timeout * 3600,  # timeout is in hours
                        max(solver_one_values + solver_two_values))
        diagonal = np.arange(0, range_end)

        plt.figure()
        plt.title(f'Time Comparision (sec)\n{self.property}')
        plt.plot(solver_one_values, solver_two_values, 'ro', diagonal, diagonal, 'b-')
        plt.xlabel(self.solvers[solver1])
        plt.ylabel(self.solvers[solver2])

        plt.savefig(os.path.join(COMPARISION_PATH,
                                 f'times_{self.solvers[solver1]}_{self.solvers[solver2]}.png'))
    def export_csv_table(self):
        assert self.result_table is not None, 'call read_result first'

        path = os.path.join(COMPARISION_PATH, f'allSolvers.csv')

        headers = ['network'] #, 'result', 'running time', '#visited states', 'avg pivoting time']
        for s in self.solvers:
            headers.append(f'{self.solvers[s]} (answer)')
            headers.append(f'{self.solvers[s]} (time)')

        data = []
        with open(path, 'w') as outfile:
            #outfile.write(f'solver: {self.solvers[solver]}\n')
            outfile.write(f'property: {self.property}\n')
            for nnet, solvers_result in sorted(self.result_table.items(),
                                               key=lambda k: self.networks[k[0]]):

                nnet_data=[self.networks[nnet]]
                for solver in self.solvers:
                    if solver in solvers_result and solvers_result[solver]:
                        nnet_data.append(solvers_result[solver].result)
                        nnet_data.append(solvers_result[solver].time)
                    else:
                        nnet_data.append('-')
                        nnet_data.append('10000')

                data.append(nnet_data)

            outfile.write(str(tabulate.tabulate(data, headers=headers)))
            outfile.write('\n')

    def export_states_comparision(self, solver1, solver2):
        assert solver1 in self.solvers
        assert solver2 in self.solvers
        assert self.result_table is not None, 'call read_result first'

        solver_one_values = []
        solver_two_values = []
        for nnet, solvers_result in self.result_table.items():
            if solvers_result.get(solver1, None) and \
                    solvers_result.get(solver2, None):
                if {solvers_result[solver1].result, solvers_result[solver1].result} <= {'SAT', 'UNSAT'}:
                    # we don't want timeouts here
                    solver_one_values.append(solvers_result[solver1].visited_states)
                    solver_two_values.append(solvers_result[solver2].visited_states)

        if not solver_one_values:
            # no matching results for the same network
            return

        range_end = max(solver_one_values + solver_two_values)
        diagonal = np.arange(0, range_end)

        plt.figure()
        plt.title(f'Visited Stated Comparision\n{self.property}')
        plt.plot(solver_one_values, solver_two_values, 'ro', diagonal, diagonal, 'b-')
        plt.xlabel(self.solvers[solver1])
        plt.ylabel(self.solvers[solver2])

        plt.savefig(os.path.join(COMPARISION_PATH,
                                 f'states_{self.solvers[solver1]}_{self.solvers[solver2]}.png'))

    def export_table(self, solver):
        assert solver in self.solvers
        assert self.result_table is not None, 'call read_result first'

        path = os.path.join(COMPARISION_PATH, f'{self.solvers[solver]}_result_table.sum')

        headers = ['network', 'result', 'running time', '#visited states', 'avg pivoting time']
        data = []
        avg = ['Average', '']
        sum_time = 0
        sum_visited_states = 0
        sum_avg_pivoting_time_mu = 0
        with open(path, 'w') as outfile:
            outfile.write(f'solver: {self.solvers[solver]}\n')
            outfile.write(f'property: {self.property}\n')
            for nnet, solvers_result in sorted(self.result_table.items(),
                                               key=lambda k: self.networks[k[0]]):
                if solver in solvers_result and solvers_result[solver]:
                    nnet_data = [
                        self.networks[nnet],
                        solvers_result[solver].result,
                        solvers_result[solver].time,
                        solvers_result[solver].visited_states,
                        solvers_result[solver].avg_pivoting_time_mu,
                    ]
                    data.append(nnet_data)

                    sum_time += solvers_result[solver].time
                    sum_visited_states += solvers_result[solver].visited_states
                    sum_avg_pivoting_time_mu += solvers_result[solver].avg_pivoting_time_mu

            # there might not be any data
            avg.append(f'{sum_time / len(data) if len(data) else 0:.3f}')
            avg.append(f'{sum_visited_states / len(data) if len(data) else 0:.3f}')
            avg.append(f'{sum_avg_pivoting_time_mu / len(data) if len(data) else 0:.3f}')

            data.append([''] * len(avg))  # add empty line
            data.append(avg)

            outfile.write(str(tabulate.tabulate(data, headers=headers)))
            outfile.write('\n')

    def export_errors(self):
        assert self.result_table is not None, 'call read_result first'

        path = os.path.join(COMPARISION_PATH, f'ERRORS')

        # so the keys will be in the same order for this function
        solvers = list(self.solvers.keys())
        headers = ['network'] + [self.solvers[s] for s in solvers]
        invalid_results = []

        for nnet, solvers_result in sorted(self.result_table.items(),
                                           key=lambda k: self.networks[k[0]]):

            # validate that every solver got the same result (SAT, UNSAT)
            results_of_nnet_set = set(r.result for r in solvers_result.values() if r is not None)
            if {'SAT', 'UNSAT'} <= results_of_nnet_set \
                    or 'ERROR' in results_of_nnet_set \
                    or 'UNKNOWN' in results_of_nnet_set:
                nnet_results = [self.networks[nnet]] + [
                    '' if solvers_result[s] is None else solvers_result[s].result
                    for s in solvers
                ]
                invalid_results.append(nnet_results)

        if invalid_results:
            # we don't want to create this file if no errors occurred
            with open(path, 'w') as outfile:
                outfile.write(str(tabulate.tabulate(invalid_results, headers=headers,
                                                    missingval='', stralign='center')))
                outfile.write('\n')

    def export_missing(self):
        assert self.result_table is not None, 'call read_result first'

        path = os.path.join(COMPARISION_PATH, f'MISSING')

        # so the keys will be in the same order for this function
        solvers = list(self.solvers.keys())
        headers = ['network'] + [self.solvers[s] for s in solvers]
        missing = []

        for nnet, solvers_result in sorted(self.result_table.items(),
                                           key=lambda k: self.networks[k[0]]):
            nnet_results = [self.networks[nnet]] + [
                'Miss' if solvers_result[s] is None
                else ''
                for s in solvers
            ]
            if any(nnet_results[1:]):
                missing.append(nnet_results)

        with open(path, 'w') as outfile:
            outfile.write(str(tabulate.tabulate(missing, headers=headers,
                                                missingval='', stralign='center')))
            outfile.write('\n')

    def export_all_results(self):
        assert self.result_table is not None, 'call read_result first'

        # using sorted so the results axis will be the same for all plots
        for solver1, solver2 in sorted(combinations(self.solvers, 2)):
            self.export_time_comparision(solver1, solver2)

        for solver1, solver2 in sorted(combinations(self.solvers, 2)):
            self.export_states_comparision(solver1, solver2)

        for solver in self.solvers:
            self.export_table(solver)

        self.export_csv_table()
        
        self.export_errors()
        self.export_missing()


if __name__ == '__main__':
    experiment_details = EXPERIMENT_DETAILS_PATH
    res = SlurmResults.from_file(experiment_details)
    res.read_results(bucket_result_files())
    res.export_all_results()

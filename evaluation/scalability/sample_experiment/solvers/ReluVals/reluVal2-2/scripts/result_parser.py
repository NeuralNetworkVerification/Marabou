import re
import sys

RES_D = {
    'No adv!': 'UNSAT',
    'adv is': 'SAT'
}


def parse(in_results):
    in_results = ''.join(in_results)
    p = re.compile(r'(?P<res>No adv!|adv is)')
    m = p.search(in_results)
    if not m:
        raise ValueError('no result')
    result = RES_D[m['res']]

    p = re.compile(r'time:\s*(?P<time>\d+\.?\d+)')
    m = p.search(in_results)
    if not m:
        raise ValueError('no time')
    time = int(float(m['time']))

    summary_line = f'{result} {time} {0} {0}\n'

    return summary_line


def convert(src_path, dest_path):
    with open(src_path, 'r') as src_file:
        in_results = src_file.readlines()
    
    try:
        summary_line = parse(in_results)

        with open(dest_path, 'w') as dest_file:
            dest_file.write(summary_line)
    except ValueError:
        with open(dest_path, 'w') as dest_file:
            dest_file.write('ERROR 0 0 0\n')


if __name__ == '__main__':
    src = sys.argv[1]
    dest = sys.argv[2]
    convert(src, dest)

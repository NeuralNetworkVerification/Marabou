#!/usr/bin/env python
import argparse
import os
import psutil
import sh
import time


def memory_kB():
    try:
        self_process = psutil.Process(os.getpid())
        children = self_process.children(recursive=True)
        mem = 0
        for proc in children:
            mem += proc.memory_info().rss / 1024
    except psutil.NoSuchProcess:
        # Process has disappeared, it's probably not using memory anymore
        mem = 0
    return mem


def end_run(path_to_out, result_str, time):
    time_spent = int(time)
    # with open(path_to_out, 'w') as results_file:
    if result_str[0] == 'D':
        with open(path_to_out + '.log', 'r') as results_file:
            lines=results_file.readlines()
            res = lines[-1] if len(lines) > 0 else "UNKNOWN"
        with open(path_to_out, 'w') as results_file:
            results_file.write(res+' '+str(time_spent) + ' 0 0\n')
    else:
        with open(path_to_out, 'w') as results_file:
            results_file.write('TIMEOUT '+str(time_spent)+' 0 0\n')


def make_dir_exist(path_to_out):
    dirname = os.path.dirname(path_to_out)
    os.makedirs(dirname, exist_ok=True)


keep_running = True
def main():
    parser = argparse.ArgumentParser(description='Run planet with a max memory usage')
    parser.add_argument('exe', type=str,
                        help='Path to the script/executable to run')
    parser.add_argument('max_mem', type=int,
                        help='Max memory usage in kB')
    parser.add_argument('max_time', type=int,
                        help='Max time to run before killing (in s)')
    parser.add_argument('out_result_path', type=str,
                        help='Where to write the results')
    parser.add_argument('others', nargs=argparse.REMAINDER)
    args = parser.parse_args()

    solver = sh.Command(args.exe)
    make_dir_exist(args.out_result_path)
    start = time.time()


    def done(cmd, success, exit_code):
        global keep_running
        keep_running = False

    p = solver(args.others, _bg=True, _done=done,
               # _out=args.out_result_path)
                _out=args.out_result_path + '.log',
                _err=args.out_result_path + '.log')

    try:
        while keep_running:
            now = time.time()
            time_spent = now - start
            if args.max_mem > 0:
                mem_usage = memory_kB()
                if mem_usage > args.max_mem:
                    end_run(args.out_result_path, "OOM", time_spent)
                    p.kill()
                    print("Killing the process - Taking too much memory.\nMEMOUT")
                    break
            if args.max_time > 0:
                if time_spent > args.max_time:
                    end_run(args.out_result_path, "timeout", time_spent)
                    p.kill()
                    print("Killing the process - Taking too much time.\nTIMEOUT ")
                    break
                time.sleep(0.05)
        else:
            # We come here if keep_running has been set to False, which means
            # that the solver has finished. We can write down its results.
            now = time.time()
            time_spent = now - start
            end_run(args.out_result_path, "Done", time_spent)
    except KeyboardInterrupt:
        p.kill()


if __name__ == '__main__':
    main()

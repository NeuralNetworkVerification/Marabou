import json
import os
import subprocess
import threading
from timeit import default_timer as timer
from pprint  import pprint

NEW_VERSION_DIR = "new_version"
ORIGINAL_VERSION_DIR = "build"
DEFAULT_TIMEOUT = 60 * 60

NET_DIR = "resources/nnet"
PROP_DIR = "resources/properties"


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
    # print(out)
    # print(err)
    return (out.strip(), err.strip(), exit_status)


def time_marabou(version_dir, net_path, property_path):
    new_start = timer()
    args = [os.path.join(version_dir, "Marabou"), net_path, property_path]

    out, err, exit = run_process(args, os.curdir, DEFAULT_TIMEOUT)
    new_end = timer()
    if exit != 0:
        new_end = 999999
    return new_end - new_start


def get_all_timings(net_paths, property_paths):
    assert len(net_paths) == len(property_paths)

    all_times = {}
    for i in range(len(net_paths)):
        cur_times = {}
        cur_times['new'] = time_marabou(NEW_VERSION_DIR, net_paths[i], property_paths[i])
        cur_times['original'] = time_marabou(ORIGINAL_VERSION_DIR, net_paths[i], property_paths[i])
        all_times[net_paths[i] + "_" + property_paths[i]] = cur_times
        pprint(cur_times)

    json.dump(all_times, open("marabou_timing.json", 'w'))
    return all_times

if __name__ == "__main__":
    paths = ((NET_DIR + '/acasxu/ACASXU_experimental_v2a_3_3.nnet', PROP_DIR + "/acas_property_3.txt"), # UNSAT 3
             (NET_DIR + '/acasxu/ACASXU_experimental_v2a_2_5.nnet', PROP_DIR + "/acas_property_2.txt"), # SAT 482
             (NET_DIR + '/acasxu/ACASXU_experimental_v2a_2_8.nnet', PROP_DIR + "/acas_property_4.txt"),  # UNSAT 308
             )

    pprint(get_all_timings([p[0] for p in paths],
                    [p[1] for p in paths]))
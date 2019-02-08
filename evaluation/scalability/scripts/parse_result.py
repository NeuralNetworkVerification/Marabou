import sys
import os
import csv

def main():
    path_name = sys.argv[1]
    cwd = os.getcwd()

    if path_name[-1] != "/":
        path_name = path_name + "/"
    if "Marabou" in path_name:
        if path_name[-8:] == "Marabou/":
            summaries = []
            for node in os.listdir(path_name):
                for prop in os.listdir("{}{}/".format(path_name, node)):
                    summaries += get_summary_marabou("{}/{}{}/{}/".format(cwd, path_name, node, prop))
        elif path_name[-10:-2] == "property":
            summaries = get_summary_marabou("{}/{}".format(cwd, path_name))
    elif "ReluVal" in path_name:
        if path_name[-8:] == "ReluVal/":
            summaries = []
            for node in os.listdir(path_name):
                for prop in os.listdir("{}{}/".format(path_name, node)):
                    summaries += get_summary_reluval("{}/{}{}/{}/".format(cwd, path_name, node, prop))
        elif path_name[-10:-2] == "property":
            summaries = get_summary_reluval("{}/{}".format(cwd, path_name))

    # Print out results
    try:
        titles = ["Solver", "network", "property", "cpus", "state", "runtime", "SAT/UNSAT"]
        with open(sys.argv[2], "w") as out_file:
            writer = csv.writer(out_file)
            writer.writerow(titles)
            for line in summaries:
                writer.writerow(line)
    except:
        titles = ["Solver", "network", "\t\t\tproperty", "cpus", "state", "runtime", "SAT/UNSAT"]
        print ("\t".join(titles))
        for line in summaries:
            print ("\t".join(line))


def get_summary_marabou(path):

    summary = [] # network, property, cpus, TO/MO/Er/Ok, runtime, SAT/UNSAT
    for filename in os.listdir(path):

        if ".err" != filename[-4:]:
            continue
        network = filename.split()[0]
        prop = "property" + filename.split()[1].split("-")[-1].split(".")[0][-1]
        cpus = filename.split(".")[-3].split("-")[-1]
        ok = False
        to = False
        mo = False
        err = False
        runtime = "0"
        SAT = "--"
        if "ACASXU" in filename and "err" in filename:
            with open(path + filename, 'r') as in_file:
                for line in in_file.readlines():
                    if "Error" in line or "error" in line or "Segmentation fault" in line:
                        err = True
                        break
                    if "[runlim] status" in line:
                        if "ok" in line:
                            ok = True
                        elif "out of time" in line:
                            to = True
                            runtime = "7200.0"
                        elif "out of memory" in line:
                            mo = True
                    if "[runlim] real" in line:
                        runtime = line.split()[-2]

        if err:
            state = "E"
        elif to:
            state = "TO"
        elif mo:
            state = "MO"
        else:
            state = "Ok"
            summary_file = path + filename.split("andrew-cav-experiment-")[-1][:-4]
            assert(os.path.isfile(summary_file))
            with open(summary_file, "r") as in_file:
                for line in in_file.readlines():
                    SAT = line.split()[0]
        if float(runtime) > 7200:
            runtime = "7200.0"
        summary.append(["Marabou", network, prop, cpus, state, runtime, SAT])

    return summary





def get_summary_reluval(path):
    summary = [] # network, property, cpus, TO/MO/Er/Ok, runtime, SAT/UNSAT
    for filename in os.listdir(path):
        if ".err" != filename[-4:]:
            continue
        network = filename.split()[0].split(".")[0]
        prop = "property" + filename.split()[1].split("-")[-3][-1]
        cpus = filename.split(".")[-3].split("-")[-1]

        ok = False
        to = False
        mo = False
        err = False
        runtime = "0"
        SAT = "--"
        if "ACASXU" in filename and "err" in filename:
            with open(path + filename, 'r') as in_file:
                for line in in_file.readlines():
                    if "Error" in line or "error" in line or "Segmentation fault" in line:
                        err = True
                        break
                    if "[runlim] status" in line:
                        if "ok" in line:
                            ok = True
                        elif "out of time" in line:
                            to = True
                            runtime = "7200.0"
                        elif "out of memory" in line:
                            mo = True
                    if "[runlim] real" in line and "limit" not in line and  not to:
                        runtime = line.split()[-2]

        if err:
            state = "E"
        elif to:
            state = "TO"
        elif mo:
            state = "MO"
        else:
            state = "Ok"
            summary_file = "{}{}.nnet-prop{}-summary-{}.txt".format(path, network, prop[-1], cpus)
            assert(os.path.isfile(summary_file))
            SAT = "UNSAT"
            with open(summary_file, "r") as in_file:
                for line in in_file.readlines():
                    if "found" in line:
                        SAT = "SAT"

        if float(runtime) > 7200:
            runtime = "7200.0"
        summary.append(["ReluVal", network, prop, cpus, state, runtime, SAT])

    return summary



if __name__ == "__main__":
    main()

import configparser
import h5py
import os
import numpy as np
import sys
import shutil

cfg = configparser.ConfigParser()
try:
    config_file = os.path.abspath(sys.argv[1])
except:
    config_file = os.path.abspath("configs/preliminary-experiment.cfg")
cfg.read(config_file)

root_dir = cfg.get('global', 'root_dir')
experiment_path = os.path.abspath(cfg.get('global', 'experiment_path')) + "/"
print("experiment path set to be {}".format(experiment_path))

benchmark_files = cfg.get('global', 'benchmark_set_file').split(',')
fs = []
summary_paths = []
for i, filename in enumerate(benchmark_files):
    assert(filename != "" )
    benchmark_set_filename = experiment_path + filename
    f = open(benchmark_set_filename, 'w')
    fs.append(f)
    print("Writing to {}".format(benchmark_set_filename))
    try:
        os.mkdir(os.path.join(experiment_path, 'summary/') + filename.split('.')[0])
    except:
        print("Invalid run name!")
    summary_paths.append(experiment_path + 'summary/' + filename.split('.')[0] + '/')

shutil.copyfile(config_file, os.path.join(experiment_path, os.path.basename(config_file)))

argses = cfg.get("global", "args").split(',')

benchmark_file = os.path.join(experiment_path, 'benchmark_set')
with open(benchmark_file, 'r') as in_file:
    for line in in_file.readlines():
        line = line.split()
        marabou_bin_path = line[0]
        benchmark = line[1]
        network_name = line[2]
        property_filename = line[3]
        config_file = line[4]
        basename = os.path.basename(property_filename).split('.')[0]
        if benchmark == 'acas':
            for k, f in enumerate(fs):
                summary_path = summary_paths[k]
                args = argses[k]
                summary_filename = os.path.join(summary_path, "acas_{}_{}".format(os.path.basename(network_name[:-5]),
                                                                                  basename))
                summary_filename = summary_filename.replace(".", "-")
                summary_filename += ".summary"
                f.write("{} acas {} {} {} {} {}\n".format(marabou_bin_path, network_name,
                                                          property_filename, summary_filename, 
                                                          config_file, args) )
        elif benchmark == 'mnist':
            for k, f in enumerate(fs):
                summary_path = summary_paths[k]
                args = argses[k]
                summary_filename = os.path.join(summary_path, "mnist_{}".format(basename))
                summary_filename = summary_filename.replace(".", "-")
                summary_filename += ".summary"
                f.write("{} mnist {} {} {} {} {}\n".format(marabou_bin_path, network_name,
                                                           property_filename, summary_filename, 
                                                           config_file, args) )
        elif benchmark == 'boeing':
            for k, f in enumerate(fs):
                summary_path = summary_paths[k]
                args = argses[k]
                summary_filename = os.path.join(summary_path, "boeing_{}".format(basename)) 
                summary_filename = summary_filename.replace(".", "-")
                summary_filename += ".summary"
                f.write("{} boeing {} {} {} {} {}\n".format(marabou_bin_path, network_name,
                                                            property_filename, summary_filename, 
                                                            config_file, args) )
        elif benchmark == 'kyle':
            for k, f in enumerate(fs):
                summary_path = summary_paths[k]
                args = argses[k]
                summary_filename = os.path.join(summary_path, "kyle_{}".format(basename)) 
                summary_filename = summary_filename.replace(".", "-")
                summary_filename += ".summary"
                f.write("{} kyle {} {} {} {} {}\n".format(marabou_bin_path, network_name,
                                                          property_filename, summary_filename, 
                                                          config_file, args) )

for f in fs:
    f.close()

for benchmark_set_filename in benchmark_files:
    shutil.copyfile(experiment_path + benchmark_set_filename, "./" + os.path.basename(benchmark_set_filename))

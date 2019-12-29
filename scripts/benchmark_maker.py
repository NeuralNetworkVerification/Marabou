import warnings
warnings.filterwarnings('ignore')
from keras.datasets import mnist
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

###########################  global config  ###################################
root_dir = cfg.get('global', 'root_dir')
experiment_path = os.path.abspath(cfg.get('global', 'experiment_path')) + "/"
try:
    os.mkdir(experiment_path)
    os.mkdir(os.path.join(experiment_path, 'Marabou'))
    os.mkdir(os.path.join(experiment_path, 'networks/'))
    os.mkdir(os.path.join(experiment_path, 'properties'))
    os.mkdir(os.path.join(experiment_path, 'properties/mnist'))
    os.mkdir(os.path.join(experiment_path, 'properties/boeing'))
    os.mkdir(os.path.join(experiment_path, 'summary/'))
except:
    print("Invalid experiment location!")
    exit()
print("experiment path set to be {}".format(experiment_path))

marabou_path = cfg.get('global', 'marabou_path')
#shutil.copytree(marabou_path + "build/", os.path.join(experiment_path, 'Marabou/build'))
#shutil.copytree(marabou_path + "maraboupy/", os.path.join(experiment_path, 'Marabou/maraboupy'))
#marabou_path = os.path.abspath(os.path.join(experiment_path, 'Marabou'))
print("Marabou path: {}".format(marabou_path))
sys.path.append(marabou_path)
from maraboupy import Marabou
marabou_bin_path = os.path.join(marabou_path, "build/Marabou")

benchmark_files = cfg.get('global', 'benchmark_set_file').split(',')
fs = []
summary_paths = []
for i, filename in enumerate(benchmark_files):
    benchmark_set_filename = experiment_path + filename 
    f = open(benchmark_set_filename, 'w')
    fs.append(f)
    print("Writing to {}".format(benchmark_set_filename))
    os.mkdir(os.path.join(experiment_path, 'summary/') + filename.split('.')[0])
    summary_paths.append(experiment_path + 'summary/' + filename.split('.')[0] + '/')

seed = int(cfg.get('global', 'random_seed'))
np.random.seed(seed)
print("Random seed {}".format(seed))

shutil.copyfile(config_file, os.path.join(experiment_path, os.path.basename(config_file)))

argses = cfg.get("global", "args").split(',')

##############################  acas config  ###################################
acas_networks_dir = cfg.get("acas", "network_dir")
acas_properties_dir = cfg.get("acas", "property_dir")
acas_properties = np.fromstring(cfg.get("acas", "properties"), dtype=int, sep=' ')
acas_number_of_benchmarks = np.fromstring(cfg.get("acas","number_of_benchmarks"), dtype=int, sep=' ')

acas_networks_dir = os.path.abspath(shutil.copytree(acas_networks_dir, experiment_path + 'networks/acas/'))
acas_properties_dir = os.path.abspath(shutil.copytree(acas_properties_dir, experiment_path + 'properties/acas/'))

print("Acas networks dir: {}".format(root_dir + acas_networks_dir))
print("Acas properties dir: {}".format(root_dir + acas_properties_dir))

acas_networks = []
for filename in os.listdir(acas_networks_dir):
    acas_networks.append(os.path.join(acas_networks_dir, filename))
for i in range(len(acas_properties)):
    print("Adding {} property {} benchmarks".format(acas_number_of_benchmarks[i], acas_properties[i]))
    property_filename = os.path.join(acas_properties_dir, "property{}.txt".format(acas_properties[i]))
    networks = np.random.choice(acas_networks, size=acas_number_of_benchmarks[i], replace=False)
    for network_filename in networks:
        for k, f in enumerate(fs):
            summary_path = summary_paths[k]
            args = argses[k]
            if "--dnc" in args:
                args_acas = args + " --divide-strategy=largest-interval"
            else:
                args_acas = args
            summary_filename = os.path.join(summary_path, "acas_net{}_prop{}".format(os.path.basename(network_filename[:-5]), acas_properties[i]))
            summary_filename = summary_filename.replace(".", "-")
            summary_filename += ".summary"
            f.write("{} acas {} {} {} {} {}\n".format(marabou_bin_path, network_filename,
                                                      property_filename, summary_filename,
                                                      config_file, args_acas))

###########################  MNIST config  ###################################
mnist_networks_dir = cfg.get("mnist", "network_dir")
mnist_architectures = cfg.get("mnist", "architectures").split()
mnist_epsilons = [np.fromstring(eps, sep=' ') for eps in cfg.get("mnist", "epsilons").split("/")]
mnist_number_of_benchmarks = [np.fromstring(nums, dtype=int, sep=' ') for nums in cfg.get("mnist", "number_of_benchmarks").split("/")]

mnist_networks_dir = os.path.abspath(shutil.copytree(mnist_networks_dir, experiment_path + 'networks/mnist/'))
mnist_properties_dir = os.path.abspath(experiment_path + 'properties/mnist/')

assert(len(mnist_epsilons) == len(mnist_architectures))
assert(len(mnist_epsilons) == len(mnist_number_of_benchmarks))

print("MNIST networks dir: {}/".format(mnist_networks_dir))
print("MNIST properties dir: {}".format(mnist_properties_dir))

def dumpMNISTTargetedAttackPropertyFile(X, epsilon, target, filename):
    # target: 0..9 corresponding to y0..y9
    with open(filename, 'w') as out_file:
        X = np.array(X).flatten() / 255
        for i, x in enumerate(X):
            out_file.write('x{} >= {}\n'.format(i, x - epsilon))
            out_file.write('x{} <= {}\n'.format(i, x + epsilon))
        for i in range(10):
            if i != target:
                out_file.write('+y{} -y{} <= 0\n'.format(i, target))

(X_train, y_train), (X_test, y_test) = mnist.load_data()
for i, arch in enumerate(mnist_architectures):
    network_name = os.path.join(mnist_networks_dir, "mnist{}.nnet".format(arch))
    print("Adding {} targeted attack benchmarks for mnist{}".format(mnist_number_of_benchmarks[i].sum(), arch))
    assert(len(mnist_epsilons[i]) == len(mnist_number_of_benchmarks[i]))
    for j, eps in enumerate(mnist_epsilons[i]):
        points = np.random.choice(range(len(y_train)), size=mnist_number_of_benchmarks[i][j], replace=False)
        for p in points:
            # find a target
            while True:
                target = np.random.choice(range(10), replace=False)
                if target != y_train[p]:
                    break
            property_filename = os.path.join(mnist_properties_dir, "net{}_ind{}_tar{}_eps{}.txt".format(arch, p, target, eps))
            dumpMNISTTargetedAttackPropertyFile(X_train[p], eps, target, property_filename)
            for k, f in enumerate(fs):
                summary_path = summary_paths[k]
                args = argses[k]
                if "--dnc" in args:
                    args_mnist = args + " --divide-strategy=split-relu"
                else:
                    args_mnist = args
                summary_filename = os.path.join(summary_path, "mnist_net{}_ind{}_tar{}_eps{}".format(arch, p, target, eps))
                summary_filename = summary_filename.replace(".", "-")
                summary_filename += ".summary"
                f.write("{} mnist {} {} {} {} {}\n".format(marabou_bin_path, network_name,
                                                           property_filename, summary_filename,
                                                           config_file, args_mnist))

###########################  boeing config  ###################################
boeing_networks_dir = cfg.get("boeing", "network_dir")
boeing_architectures = cfg.get("boeing", "architectures").split()
boeing_epsilons_deltas = [[(float(pair.split(',')[0]), float(pair.split(',')[1])) for pair in pairs.split()]
                          for pairs in cfg.get("boeing", "epsilons_deltas").split("/")]
boeing_number_of_benchmarks = [np.fromstring(nums, dtype=int, sep=' ') for nums in cfg.get("boeing", "number_of_benchmarks").split("/")]
boeing_training_data_dir = cfg.get('boeing', 'training_data_dir')

boeing_networks_dir = os.path.abspath(shutil.copytree(boeing_networks_dir, experiment_path + 'networks/boeing/'))
boeing_properties_dir = os.path.abspath(experiment_path + 'properties/boeing/')

#assert(len(boeing_epsilons_deltas) == len(boeing_architectures))
#assert(len(boeing_epsilons_deltas) == len(boeing_number_of_benchmarks))

print("Boeing networks dir: {}".format(boeing_networks_dir))
print("Boeing properties dir: {}".format(boeing_properties_dir))

def dump_boeing_properties_file(X, y, eps, delta, boeing_properties_filename):
    with open(boeing_properties_filename, 'w') as in_file:
        in_file.write("from maraboupy import Marabou\n")
        in_file.write("import numpy as np\n\n")
        in_file.write("def encode_property( network ):\n")
        height = X.shape[0]
        width = X.shape[1]
        for h in range(height):
            for w in range(width):
                in_file.write("\tnetwork.setLowerBound(network.inputVars[0][0,{},{},0],{})\n".format(h, w, X[h,w,0] - eps))
                in_file.write("\tnetwork.setUpperBound(network.inputVars[0][0,{},{},0],{})\n".format(h, w, X[h,w,0] + eps))
        in_file.write("\tMarabou.addEquality(network,network.inputVars[0].reshape({}),np.ones({}),{}*0.5)\n".format(height*width, height*width, height*width))
        in_file.write("\tnetwork.setUpperBound(network.outputVars[0][0], {})\n".format(y[0]-delta))
        in_file.write("\treturn network")
    return

for i, arch in enumerate(boeing_architectures):
    load_file = os.path.join(boeing_training_data_dir, arch.split("_")[0] + ".h5")
    training_data_file = h5py.File(load_file,'r')
    X_train = np.array(training_data_file['X_train'])
    training_data_file.close()
    if len(X_train.shape)<4:
        X_train = X_train.reshape(X_train.shape + (1,))

    network_name = os.path.join(boeing_networks_dir, "{}.pb".format(arch))
    network = Marabou.read_tf(network_name)

    print("Adding {} targeted attack benchmarks for {}".format(boeing_number_of_benchmarks[i].sum(), arch))
    assert(len(boeing_epsilons_deltas[i]) == len(boeing_number_of_benchmarks[i]))
    for j, eps_delta in enumerate(boeing_epsilons_deltas[i]):
        inds = np.random.choice(range(len(X_train)), size=boeing_number_of_benchmarks[i][j], replace=False)
        eps = eps_delta[0]
        delta = eps_delta[1]
        for ind in inds:
            y = network.evaluateWithoutMarabou(X_train[ind].reshape((1,)+X_train[ind].shape))
            boeing_properties_filename = os.path.join(boeing_properties_dir, "net{}_ind{}_eps{}_delta{}".format(arch, ind, eps, delta))
            boeing_properties_filename = boeing_properties_filename.replace(".", "-")
            boeing_properties_filename += ".py"
            dump_boeing_properties_file(X_train[ind], y, eps, delta, boeing_properties_filename)
            for k, f in enumerate(fs):
                summary_path = summary_paths[k]
                args = argses[k]
                if "--dnc" in args:
                    args_boeing = args
                else:
                    args_boeing = args

                summary_filename = os.path.join(summary_path, "boeing_net{}_ind{}_eps{}_delta{}".format(arch, ind, eps, delta)) 
                summary_filename = summary_filename.replace(".", "-")
                summary_filename += ".summary"
                f.write("{} boeing {} {} {} {} {}\n".format(marabou_bin_path, network_name,
                                                            boeing_properties_filename, summary_filename, 
                                                            config_file, args_boeing ) )
for f in fs:
    f.close()

for benchmark_set_filename in benchmark_files:
    shutil.copyfile(experiment_path + benchmark_set_filename, "./" + os.path.basename(benchmark_set_filename))

#  Marabou
[![Marabou](https://github.com/NeuralNetworkVerification/Marabou/actions/workflows/ci.yml/badge.svg)](https://github.com/NeuralNetworkVerification/Marabou/actions/workflows/ci.yml)
[![codecov.io](https://codecov.io/github/NeuralNetworkVerification/Marabou/coverage.svg?branch=master)](https://codecov.io/github/NeuralNetworkVerification/Marabou?branch=master)

Deep neural networks are proliferating in many applications. Consequently, 
there is a pressing need for tools and techniques for network analysis and 
certification. To help address that need, we present Marabou, a framework for
verifying deep neural networks. 

Marabou is an SMT-based tool that can answer queries over networks. A typical DNN 
verification query consists of two parts: (i) a neural network,  and (ii) a property
to be checked; and its result is either a formal guarantee that the network satisfies 
the property, or a concrete input for which the property is violated (a counter-example).

Marabou uses ONNX as its main network format. It supports feed-forward NNs with a 
wide range of activations. It also supports the .nnet and the Tensorflow NN formats.

Properties can be specified using the Python interface. In addition, Marabou also supports 
the VNNLIB property format. 

For more details about the design and features of version 2.0 of Marabou, check out
the latest [tool paper](https://arxiv.org/pdf/2401.14461.pdf). The initial version
of Marabou is described in a [previous tool paper](https://aisafety.stanford.edu/marabou/MarabouCAV2019.pdf).

## Research

Marabou is a research product. More information about publications involving Marabou 
can be found [here](https://neuralnetworkverification.github.io/).

## Installation

### Installing via Pip

The recommended way to install Marabou is via `pip` using the command
```bash
pip install maraboupy
```
which will install both the `Marabou` executable on your path and the Python bindings.
The Python interface currently supports Python 3.8, 3.9, 3.10 and 3.11.

### Building from source

#### Build dependencies

The build process uses CMake version 3.16 (or later).
You can get CMake [here](https://cmake.org/download/).

Marabou depends on the Boost and the OpenBLAS libraries, which are automatically 
downloaded and built when you run make. Library CXXTEST comes included in the 
repository.

The current version of Marabou can be built for Linux or MacOS machines using CMake:
```bash
git clone https://github.com/NeuralNetworkVerification/Marabou/
cd Marabou/
mkdir build 
cd build
cmake ../
cmake --build ./
```
To enable multiprocess build change the last command to:
```bash
cmake --build . -j PROC_NUM
```
After building Marabou, the compiled binary is located at `build/Marabou`, and the 
shared library for the Python API is located in `maraboupy/`. Building the Python 
interface requires *pybind11* (which is automatically downloaded).

Export `Marabou` folder to Python and Jupyter paths:
```
export PYTHONPATH=PYTHONPATH:/path/to/marabou/folder
export JUPYTER_PATH=JUPYTER_PATH:/path/to/marabou/folder
```
and the built `maraboupy` is ready to be used from a Python or a Jupyter script.

By default we install a 64bit Marabou and consequently a 64bit Python interface, 
the `maraboupy/build_python_x86.sh` file builds a 32bit version.

#### Compile Marabou with the Gurobi optimizer (optional)
Marabou can be configured to use the Gurobi optimizer, which can replace the
in-house LP solver and enable a few additional solving modes.

Gurobi requires a license (a free academic license is available), after 
getting one the software can be downloaded [here](https://www.gurobi.com/downloads/gurobi-optimizer-eula/) 
and [here](https://www.gurobi.com/documentation/11.0/quickstart_linux/software_installation_guid.html#section:Installation) are
installation steps, there is a [compatibility issue](https://support.gurobi.com/hc/en-us/articles/360039093112-C-compilation-on-Linux)
that should be addressed.
A quick installation reference:
```bash
export INSTALL_DIR=/opt
sudo tar xvfz gurobi11.0.3_linux64.tar.gz -C $INSTALL_DIR
cd $INSTALL_DIR/gurobi1103/linux64/src/build
sudo make
sudo cp libgurobi_c++.a ../../lib/
```
Next, set the following environment variables (e.g., by adding the following to the `.bashrc` and invoke `source .bashrc`): 
```bash
export GUROBI_HOME="/opt/gurobi1103/linux64"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
export GRB_LICENSE_FILE=/path/to/license.lic
```

After Gurobi is set up, follow the same build instruction of Marabou in the beginning, 
except that you need  to run the following command instead of `cmake ../`:
```bash
cmake ../ -DENABLE_GUROBI=ON
```

#### Other CMake options

The `cmake ../` command can take other options, for example:

- Compile without the Python binding:
```bash
cmake ../ -DBUILD_PYTHON=OFF
```
- Compile in debug mode (default is release):
```bash
cmake ../ -DCMAKE_BUILD_TYPE=Debug
```

#### Testing

To run tests we use [ctest](https://cmake.org/cmake/help/v3.15/manual/ctest.1.html).
The tests have labels according to level (unit/system/regress0/regress1...), and the code they are testing (engine/common etc...).  
For example to run all unit tests execute in the build directory:
```bash
ctest -L unit
```
On every build we run the unit tests, and on every pull request we run unit,
system, regress0 and regress1.

Another option to build and run all of the tests is: 
```bash
cd path/to/marabou/repo/folder
mkdir build 
cd build
cmake ..
make check -j PROC_NUM
```

### Build Instructions for Windows using Visual Studio

We no longer provide Windows support. Instructions to build an old version of Marabou
for Windows can be found [here](https://github.com/NeuralNetworkVerification/Marabou/tree/0fc1d10ff0e1859cf32abe54eb22f3ec0fec59f6?tab=readme-ov-file#build-instructions-for-windows-using-visual-studio).

## Running Marabou

### Run from the Command line

The `Marabou` executable can be called directly from the command line.
It takes as arguments the network to verified and the property. `Marabou --help` would
return a list of available options. 

The repository contains sample networks and properties in the *resources* folder.
To run Marabou on such an example, you can execute the following command
from the repo directory:
```bash
Marabou resources/nnet/acasxu/ACASXU_experimental_v2a_2_7.nnet resources/properties/acas_property_3.txt
```
(if built from source, then you will need to replace `Marabou` with the path to the built executable
i.e. `build/Marabou`).

### Run using Python

The `maraboupy/examples` folder contains a list of examples. Please also see our 
[documentation](https://neuralnetworkverification.github.io/Marabou/) for the python interface, 
which contains examples, API documentation, and a developer's guide.

### Advanced configuration

#### Choice of solver configurations

Currently the default configuration of Marabou is a *single-threaded* one that
uses DeepPoly analysis for bound tightening, and the DeepSoI procedure during the complete search.
For optimal runtime performance, you need to build Marabou with Gurobi enabled (See sub-section below for Gurobi installation),
so that LPs are solved by Gurobi instead of the open-source native simplex engine.  

You could also leverage *parallelism* by setting the num-workers option to N. This will spawn N threads, each solving
the original verification query using the single-threaded configuration with
a different random seed. This is the preferred parallelization strategy for low level of parallelism (e.g. N < 30).
For example to solve a query using this mode with 4 threads spawned:
```
./resources/runMarabou.py resources/nnet/mnist/mnist10x10.nnet resources/properties/mnist/image3_target6_epsilon0.05.txt --num-workers=4
```

If you have access to a large number of threads, you could also consider the Split-and-Conquer mode (see below).

#### Using the Split and Conquer (SNC) mode

In the SNC mode, activated by *--snc* Marabou decomposes the problem into *2^n0*
sub-problems, specified by *--initial-divides=n0*. Each sub-problem will be
solved with initial timeout of *t0*, supplied by *--initial-timeout=t0*. Every
sub-problem that times out will be divided into *2^n* additional sub-problems,
*--num-online-divides=n*, and the timeout is multiplied by a factor of *f*,
*--timeout-factor=f*. Number of parallel threads *t* is specified by
*--num-workers=t*.

So to solve a problem in SNC mode with 4 initial splits and initial timeout of 5s, 4 splits on each timeout and a timeout factor of 1.5:
```
build/Marabou resources/nnet/acasxu/ACASXU_experimental_v2a_2_7.nnet resources/properties/acas_property_3.txt --snc --initial-divides=4 --initial-timeout=5 --num-online-divides=4 --timeout-factor=1.5 --num-workers=4
```

A guide to Split and Conquer is available as a Jupyter Notebook in [resources/SplitAndConquerGuide.ipynb](resources/SplitAndConquerGuide.ipynb).

## Developing Marabou

### Setting up your development environment

1. Install `pre-commit` which manages the pre-commit hooks and use it to install the hooks, e.g.
```bash
pip install pre-commit
pre-commit install
```
This guarantees automatic formatting of your C++ code whenever you commit.

### Tests

We have three types of tests:

1. Unit tests - test specific small components, the tests are located alongside the code in a _tests_ folder (for example: `src/engine/tests`), to add a new set of tests, add a file named *Test_FILENAME* (where *FILENAME* is what you want to test), and add it to the `CMakeLists.txt` file (for example `src/engine/CMakeLists.txt`)

2. System tests - test an end to end use case but still have access to internal functionality. Those tests are located in `src/system_tests`. To add new set of tests create a file named *Test_FILENAME*, and add it also to `src/system_tests/CMakeLists.txt`.

3. Regression tests - test end to end functionality thorugh the API, each test is defined by:
  * network_file - description of the "neural network" supporting nnet and mps formats (using the extension to decdie on the format)  
  * property_file - optional, constraint on the input and output variables  
  * expected_result - sat/unsat  
  The regression tests are divided into 5 levels to allow variability in running time.
  To add a new regression tests:
  * add the description of the network and property to the _resources_ sub-folder 
  * add the test to: _regress/regressLEVEL/CMakeLists.txt_ (where LEVEL is within 0-5) 

4. Python tests - test the `maraboupy` package.

In each build we run unit and system tests, and on pull request we also run the Python tests and levels 0 & 1 of the regression tests.
In the future we will run other levels of regression weekly / monthly.

## Acknowledgments

The Marabou project acknowledges support from the Binational Science Foundation (BSF) (2017662, 2021769, 2020250), the Defense Advanced Research Projects Agency (DARPA) (FA8750-18-C-0099), the European Union (ERC, VeriDeL, 101112713), the Federal Aviation Administration (FAA), Ford Motor Company (Alliance award 199909), General Electric (GE) Global Research, Intel Corporation, International Business Machines (IBM), the Israel Science Foundation (ISF) (683/18, 3420/21), the National Science Foundation (NSF) (1814369, 2211505, DGE-1656518), the Semiconductor Research Corporation (SRC) (2019-AU-2898), Siemens Corporation, the Stanford Center for AI Safety, the Stanford CURIS program, and the Stanford Institute for Human-Centered Artificial Intelligence (HAI).


Marabou is used in a number of flagship projects at [Stanford's AISafety
center](http://aisafety.stanford.edu).

## People

Authors and contributors to the Marabou project can be found in AUTHORS and THANKS files, respectively.

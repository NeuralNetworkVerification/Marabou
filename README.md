Marabou
===============================================================================
Marabou is a tool for verification of deep neural networks (DNNs) with piece-wise
linear functions. It checks reachability and robustness properties on a given
network. It is based on and extends the
[ReluPlex](https://github.com/guykatzz/ReluplexCav2017) algorithm, replacing
GLPK with its own simplex core enabling tighter integration with satisfiability
modulo theory (SMT) techniques.

Marabou supports fully connected feed-forward and convolutional NNs with
piece-wise linear activation functions, in the .nnet and TensorFlow formats.
Properties can be specified using inequalites over input and output variables.
For more details about the features of Marabou check out the [tool
paper](marabouCAV2019.pdf) or the [slides](slides.pdf). For more information
about the input formats please check the [wiki](TODO:formats_page).

Download
------------------------------------------------------------------------------
The latest version of Marabou is available on GitHub:
[http://github.com/GuyKatzz/Marabou].

Static binaries are available from ...

Build and Dependencies
------------------------------------------------------------------------------
Marabou can be built on Linux and macOS. Marabou depends on the Boost library
which is automatically downloaded and built when you run make. Library CXXTEST
comes included in the repository.

Using Marabou through the Python interface requires Python 3.5(?). If you have a
higher version installed you can set up a Python virtual environment, see
[here](https://docs.python.org/3/tutorial/venv.html) for more information.


To build Marabou, run the following:
```
cd pathToMarabouFolder
make
```
The binary will be produced in ...

To use Python interface it needs to be built, so in addition to building Marabou run:
```
cd maraboupy
make
```

Export maraboupy folder to Python and Jupyter paths:
```
PYTHON_PATH=PYTHON_PATH:/pathToMarabouFolder/maraboupy
JUPYTER_PATH=JUPYTER_PATH:/pathToMarabouFolder/maraboupy
```
and Marabou is ready to be used from a python or a jupyter script.

Getting Started
-----------------------------------------------------------------------------
### To run Marabou from Command line 
After building Marabou the binary is located at ... The repository contains sample networks and properties in ... 

```
/path/to/bin/marabou.elf resources/networks/acas/? resources/properties/acas_property_3.txt
```


### Jupyter notebook using Python interface 
An example jupyter notebook is available at ... It is ready to run assuming the python interface is build and the jupyter and python paths exported correctly.

Related projects
-----------------------------------------------------------------------------
Marabou is used for a number of flagship projects at [Stanford's AISafety
center](http://aisafety.stanford.edu).

People
-----------------------------------------------------------------------------
[Clark Barrett](http://theory.stanford.edu/~barrett/)

[Guy Katz](https://www.katz-lab.com/)

[Aleksandar Zeljic](https://profiles.stanford.edu/aleksandar-zeljic)

[Ahmed Irfan](https://profiles.stanford.edu/ahmed-irfan)

[Haoze Wu]()

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
paper](marabouCAV2019.pdf) or the [slides](marabouSlides.pdf). For more
information about the input formats please check the
[wiki](https://github.com/guykatzz/Marabou/wiki/Marabou-Input-Formats).

Download
------------------------------------------------------------------------------
The latest version of Marabou is available on (GitHub)[http://github.com/GuyKatzz/Marabou].

## Static binaries

Pre-compiled binaries are available for Linux and MacOS:

[marabou-1.0-x86_64-linux.zip](https://aisafety.stanford.edu/marabou/marabou-1.0-x86_64-linux.zip)
[marabou-1.0-x86_64-MacOS.zip](https://aisafety.stanford.edu/marabou/marabou-1.0-x86_64-MacOS.zip)

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
The compiled binary will be *./src/engine/marabou.elf*. 

To build the Python interface, in addition to building Marabou, run:
```
cd maraboupy
make
```

Export maraboupy folder to Python and Jupyter paths:
```
PYTHON_PATH=PYTHON_PATH:/pathToMarabouFolder/maraboupy
JUPYTER_PATH=JUPYTER_PATH:/pathToMarabouFolder/maraboupy
```
and Marabou is ready to be used from a Python or a Jupyter script.

Getting Started
-----------------------------------------------------------------------------
### To run Marabou from Command line 
After building Marabou the binary is located at ./src/engine/marabou.elf. The
repository contains sample networks and properties in the *experiments* folder.
For more information see [experiments/README.md](experiments/README.md). To use
Marabou on an 

```
./src/engine/marabou.elf experiments/networks/acas/ACASXU_experimental_v2a_2_7.nnet experiments/properties/acas_property_3.txt
```


### Using Python interface 
The *maraboupy/examples* folder contains several python scripts and Jupyter notebooks that can be used An example Jupyter notebook is available [here](maraboupy/examples/Example Notebook.ipynb). 

### About formats and properties
To find out more about supported formats and how to specify network properties see [experiments/README.md](experiments/README.md)
Acknowledgments
-----------------------------------------------------------------------------
Marabou is sponsored by NSF, DARPA, Intel, Siemens, Ford, GE.


Marabou is used in a number of flagship projects at [Stanford's AISafety
center](http://aisafety.stanford.edu).



People
-----------------------------------------------------------------------------
[Guy Katz](https://www.katz-lab.com/)
[Clark Barrett](http://theory.stanford.edu/~barrett/)
[Aleksandar Zeljic](https://profiles.stanford.edu/aleksandar-zeljic)
[Ahmed Irfan](https://profiles.stanford.edu/ahmed-irfan)
[Haoze Wu](http://www.haozewu.com/)
[Christopher Lazarus](https://profiles.stanford.edu/christopher-lazarus-garcia)
[Kyle Julian](https://www.linkedin.com/in/kyjulian) 
[Chelsea Sidrane](https://www.linkedin.com/in/chelseasidrane)
[Parth Shah](https://www.linkedin.com/in/parthshah1995)
[Shantanu Thakoor](https://in.linkedin.com/in/shantanu-thakoor-4b2630142) 
[Rachel Lim](https://rachellim.github.io/)
[Derek A. Huang]() 
[Duligur Ibeling]()

#  Marabou

Marabou supports fully connected feed-forward and convolutional NNs with
piece-wise linear activation functions, in the .nnet and TensorFlow formats.
Properties can be specified using inequalites over input and output variables.
For more details about the features of Marabou check out the [tool
paper](aisafety.stanford.edu/marabou/MarabouCAV2019.pdf) or the [slides](aisafety.stanford.edu/marabou/MarabouSlides.pdf). For more
information about the input formats please check the
[wiki](https://github.com/guykatzz/Marabou/wiki/Marabou-Input-Formats).

Download
------------------------------------------------------------------------------
The latest version of Marabou is available on (GitHub)[http://github.com/GuyKatzz/Marabou].

## Static binaries

Pre-compiled binary for Linux is available:

[marabou-1.0-x86_64-linux.zip](https://aisafety.stanford.edu/marabou/marabou-1.0-x86_64-linux.zip)

Build and Dependencies
------------------------------------------------------------------------------
Marabou can be built on Linux and macOS. Marabou depends on the Boost library
which is automatically downloaded and built when you run make. Library CXXTEST
comes included in the repository.

Using Marabou through the Python interface requires Python 3. It may be useful
to set up a Python virtual environment, see
[here](https://docs.python.org/3/tutorial/venv.html) for more information.


To build Marabou, run the following:
```
cd path/to/marabou/repo/folder
make
```
The compiled binary will be in *./src/engine/marabou.elf*. 

To build the Python interface, in addition to building Marabou, run:
```
cd maraboupy
make
```

Export maraboupy folder to Python and Jupyter paths:
```
PYTHONPATH=PYTHONPATH:/path/to/marabou/folder
JUPYTER_PATH=JUPYTER_PATH:/path/to/marabou/folder
```
and Marabou is ready to be used from a Python or a Jupyter script.

Getting Started
-----------------------------------------------------------------------------
### To run Marabou from Command line 
After building Marabou the binary is located at *src/engine/marabou.elf*. The
repository contains sample networks and properties in the *resources* folder.
For more information see [resources/README.md](resources/README.md).

To run Marabou, execute from the repo directory, for example:

```
src/engine/marabou.elf resources/nnet/acasxu/ACASXU_experimental_v2a_2_7.nnet resources/properties/acas_property_3.txt
```


### Using Python interface 
The *maraboupy/examples* folder contains several python scripts and Jupyter
notebooks that can be used starting points. 

### Using the Divide and Conquer (DNC) mode
In the DNC mode, activated by *--dnc* Marabou decomposes the problem into *n0*
sub-problems, specified by *--initial-divides=n0*. Each sub-problem will be
solved with initial timeout of *t0*, supplied by *--initial-timeout=t0*. Every
sub-problem that times out will be divided into *n* additional sub-problems,
*--num-online-divides=n*, and the timeout is multiplied by a factor of *f*,
*--timeout-factor=f*. Number of parallel threads *t* is specified by
*--num-workers=t*.

So to solve a problem in DNC mode with 4 initial splits and initial timeout of 5s, 4 splits on each timeout and a timeout factor of 1.5:
```
src/engine/marabou.elf resources/nnet/acasxu/ACASXU_experimental_v2a_2_7.nnet resources/properties/acas_property_3.txt --dnc --initial-divides=4 --initial-timeout=5 --num-online-divides=4 --timeout-factor=1.5 --num-workers=4
```

Acknowledgments
-----------------------------------------------------------------------------

The Marabou project is partially supported by grants from the Binational Science
Foundation (2017662), the Defense Advanced Research Projects Agency
(FA8750-18-C-0099), the Semiconductor Research Corporation (2019-AU-2898), the
Federal Aviation Administration, Ford Motor Company, Intel Corporation, the
Israel Science Foundation (683/18), the National Science Foundation (1814369,
DGE-1656518), Siemens Corporation, General Electric, and the Stanford CURIS program.


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
Derek A. Huang 
Duligur Ibeling
Elazar Cohen
Ben Goldberger
Omri Cohen

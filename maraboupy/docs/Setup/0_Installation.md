# Installation

Maraboupy is the Python API for Marabou, which is written in C++.
Marabou and Maraboupy must first be built from source using CMake.
First, clone the Marabou repository:
```
git clone https://github.com/NeuralNetworkVerification/Marabou.git
```

The marabou build process uses CMake version 3.2 (or later).
You can get CMake [here](https://cmake.org/download/).

Building Marabou and Maraboupy also depends on Boost and pybind11, which are
downloaded automatically during the build process.

The python interface was tested only on versions >3.5 and >2.7. The build process prefers python3 but will work if there is only python 2.7 available. (To control the default change the DEFAULT_PYTHON_VERSION variable).  
By default, building Marabou also builds the python API, Maraboupy. 
The BUILD_PYTHON variable controls whether or not the python API is built,
and the PYTHON_EXECUTABLE variable can control the python executable used to build Maraboupy.
This process will produce the binary file and the shared library for the Python 
API. 

Marabou can be built for Linux, MacOS, or Windows machines.

## Marabou build instructions for Linux or MacOS

To build build both Marabou and Maraboupy using CMake, run:
```
cd path/to/marabou/repo/folder
mkdir build 
cd build
cmake .. -DBUILD_PYTHON=ON
cmake --build .
```

To enable multiprocess build change the last command to:
```
cmake --build . -j PROC_NUM
```
To compile in debug mode (default is release)
```
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_PYTHON=ON
```

The compiled Marabou binary will be in the *build* directory, named _Marabou_.
In addition, a shared library will be created and placed in the maraboupy directory, where the
python sources files are located. Check to see that the MarabouCore shared library is present in the 
maraboupy directory after building.

After building, add the Marabou root directory to your PYTHONPATH environmental variable.

## Build Instructions for Windows using Visual Studio

First, install Visual Studio 2017 or later and select the "Desktop development with C++" workload. 
Ensure that CMake is installed and added to your PATH.

Open a command prompt and run:
```
cd path\to\marabou\repo\folder
mkdir build 
cd build
cmake .. -G"Visual Studio 15 2017 Win64" -DBUILD_PYTHON=ON
cmake --build . --config Release
```
This process builds Marabou using the generator "Visual Studio 15 2017 Win64". 
For 32-bit machines, omit Win64. Other generators and older versions of Visual Studio can likely be used as well, 
but only "Visual Studio 15 2017 Win64" has been tested.

The Marabou executable file will be written to the build/Release folder. To build in 
Debug mode, simply run "cmake --build . --config Debug", and the executables will be 
written to build/Debug.

In addition, a shared library will be created for the python API, Maraboupy, and placed in either
maraboupy/Release or maraboupy/Debug folders, depending on build type. However, Maraboupy needs to have
the shared library located in maraboupy alongside the python sources files, so you will need to run either
```
cp maraboupy/Release/* maraboupy
```
or 
```
cp maraboupy/Debug/* maraboupy
```
depending on build type in order to copy the shared library to the maraboupy folder.

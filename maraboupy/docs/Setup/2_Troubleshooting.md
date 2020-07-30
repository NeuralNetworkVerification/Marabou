# Troubleshooting

If problems are encountered while building or testing Marabou or Maraboupy, here are some possible common causes to investigate.

## Ensure correct python executable is used
When you run cmake with a command like
```
cmake .. -DBUILD_PYTHON=ON
```
there should be a line such as "Found PythonInterp: /path/to/python". Make sure that this python executable file is the one you 
want to use, and that the python version is at least 3.5 for python3 or at least 2.7 for python2. If Marabou is built with a different
python executable than the version you want to use, then you may run into some errors using Marabou.

To force CMake to use a particular python executable file instead of the one it found on its own, use
```
cmake .. -DBUILD_PYTHON=ON -DPYTHON_EXECUTABLE=/path/to/python
```
where /path/to/python is the path to the python executable you want to use. This may be important for Windows users to make sure
that windows python is used rather than cygwin python.

## 32bit python
By default we install a 64bit Marabou and consequently a 64bit python interface. 
The maraboupy/build_python_x86.sh file builds a 32bit version.

## Comparison to Travis output
If you are still having issues, you can look at how Travis is set up as well as the outputs of Travis to notice any differences
in your setup or build output. Travis is configured in the .travis.yml file in the Marabou root directory. Travis creates
three different builds: Linux OS with gcc compiler, Linux OS with clang compiler, and Windows OS. Travis shows how Marabou is 
built and tested. The output from Travis can be found at [here](https://travis-ci.com/github/NeuralNetworkVerification/Marabou).
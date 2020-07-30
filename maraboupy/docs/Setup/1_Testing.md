# Testing

After Marabou and Maraboupy have been built, it is recommended to run some of the tests
to make sure everything was built correctly and functions properly. Tests can be run
directly on the Marabou executable, or in python to test the Maraboupy shared library.

## Marabou executable tests
We have three types of Marabou tests:
* **unit tests**: test specific small components, the tests are located alongside the code in a tests folder (for example: src/engine/tests), to add a new set of tests, add a file named Test_FILENAME (where FILENAME is what you want to test), and add it to the CMakeLists.txt file (for example src/engine/CMakeLists.txt)
* **system tests**: test an end to end use case but still have access to internal functionality. Those tests are located in src/system_tests. To add new set of tests create a file named Test_FILENAME, and add it also to src/system_tests/CMakeLists.txt.
* **regression tests**: test end to end functionality thorugh the API, each test is defined by:
  - network_file: description of the "neural network" supporting nnet and mps formats (using the extension to decdie on the format)
  - property_file: optional, constraint on the input and output variables
  - expected_result: sat/unsat
  
Regression tests are divided into 6 levels to allow variability in running time. To add a new regression test:
* add the description of the network and property to the resources sub-folder
* add the test to: regress/regressLEVEL/CMakeLists.txt (where LEVEL is within 0-5). In each build we run unit_tests and system_tests, on pull request we run regression 0 & 1, in the future we will run other levels of regression weekly / monthly. 

To run unit tests, execute from the build directory:
```
ctest -L unit
```

To execute system tests, execute from the build directory:
```
ctest -L system
```

To execute regression tests, execute from the build directory:
```
ctest -L regress[LEVEL]
```
where LEVEL should be replaced with the regression level, an integer between 0-5.

These tests can be run in parallel by adding -j PROC_NUM to the end of a ctest command, where PROC_NUM is the number
of processes to run in parallel.
These tests should pass if the Marabou has been built correctly. These tests are not necessary to run but may help to identify issues 
before using Marabou or Maraboupoy.

## Maraboupy python tests
Tests have been added for Maraboupy via pytest and are located in the maraboupy/test folder. 
To run these tests, ensure that pytest and numpy have been installed:
```
pip install pytest numpy
```

To run tests on the tensorflow parser, ensure that tensorflow is installed. The tensorflow parser can accomodate tensorflow 
versions at least 1.13, including tensorflow versions greater than 2.0. If not installed, tensorflow can be installed with
```
pip install tensorflow
```

To run tests with the ONNX parser, both onnx and onnxruntime need to be installed:
```
pip install onnx onnxruntime
```

Note that installing tensorflow and onnx are optional. If these packages are not installed, 
Maraboupy will still function, and a warning will be given that those particular parsers will not be available until the respective
packages are installed. Recommended packages for testing are described in maraboupy/test_requirements.txt, and can be installed all at once with
```
pip install -r maraboupy/test_requirements.txt
```

To run the tests, run from the maraboupy directory:
```
python -m pytest test
```

Note that this will attempt to run all tests, and there may be failures if a python package is missing, such as tensorflow or onnx. 
These failures are ok if you do not with to use those parsers. To run an individual test, specify the test file instead of the folder,
such as 
```
python -m pytest test/test_nnet.py
```

If all tests fail, then Maraboupy has not been built correctly. Make sure that a MarabouCore shared library file has been created and placed in the maraboupy folder, and that the Marabou root directory has been added to the python path.
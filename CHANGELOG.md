# Changelog

## Next Release
* Added support for ONNX networks with tanh nodes to command-line interface
* Added proof producing versions of Sign, Max, Absolute Value and Disjunction constraints
* Added support for properties provided in VNN-LIB format for ONNX networks via the Python API
* Fixed bugs when parsing Sigmoid layers in the C++ ONNX parser.
* Supported additional non-linear constraints Softmax and Bilinear
* Removed dependency on torch and drop support for Python3.7
* Bumped ONNX version to >= 1.15.0
* Added support for Leaky ReLU
* Added support for Pythonic API
* Command-line ONNX parser now supports networks with multiple outputs.
* Command-line ONNX parser now supports the following operators: Cast, Squeeze, LeakyRelu.
* Errors now are printed on `stderr` rather than `stdout`
* Not reindexing output variables to immediately follow after input variables in the MarabouNetworkONNX class

## Version 1.0.0
* Initial versioned release

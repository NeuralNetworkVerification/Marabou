# Changelog

## Next Release
  - Adding incremental infrastructure which allows pushing and popping constraints to/from the InputQuery.
  - Dropped support for parsing Tensorflow network format. Newest Marabou version that supports Tensorflow is at commit 190555573e4702.
  - Fixed bug in the parsing of `transpose` nodes in command line C++ parser.
  - Implemented forward-backward abstract interpretation, symbolic bound tightening, interval arithmetic and simulations for all activation functions.
  - Added the BaBSR heuristic as a new branching strategy for ReLU Splitting
  - Support Sub of two variables, "Mul" of two constants, Slice, and ConstantOfShape in the python onnx parser

## Version 2.0.0

* Changes in core solving module:
  - Added proof producing versions of `Sign`, `Max`, `Absolute Value` and `Disjunction` constraints.
  - Added support for `LeakyRelu`, `Clip`, `Round`, `Softmax`.
  - Added support for forward-backward abstract interpretation.

* Dependency changes:
  - Dropped support for Python 3.7
  - Now use ONNX 1.15.0 (up from 1.12.0) in both C++ and Python backends.
  - The class `MarabouONNXNetwork` no longer depends on `torch` in Python backend.
  - Upgrade C++ standard from 11 to 17.

* Marabou now prints errors on `stderr` rather than `stdout`

* Changes to command-line ONNX support:
  - Fixed bug with variable lower bounds not being set correctly.
  - Fixed bug with sigmoid operators not being parsed correctly.
  - Added support for `Tanh`, `Unsqueeze`, `Squeeze`, `LeakyRelu`, `Dropout`, and `Cast` operators.
  - Added support for networks with multiple outputs

* Added command-line support for properties in the VNNLIB format.

* Changes to Python ONNX support:
  - Added support for `Softmax`, `Bilinear`, `Dropout`, and `LeakyRelu` operators.
  - `MarabouONNXNetwork` no longer exposes the fields `madeGraphEquations`, `varMap`, `constantMap`, `shapeMap`
    as these were supposed to be internal implementation details.
  - `MarabouONNXNetwork` no longer has a `shallowCopy` method. Instead of calling this method,
    you should set the new parameter `preserveExistingConstraints` in the method `readONNX` to
    `True` which has the same effect.
  - The constructor `MarabouONNXNetwork()` and method `MarabouNetwork.readONNX` no longer take
    a `reindexOutputVars` parameter (was intended to be used for internal testing purposes only).
  - The method `MarabouNetwork.getMarabouQuery` has been renamed `getInputQuery`.

* Added support for creating constraints using the overloaded syntax `<=`, `==` etc. in
  the Python backend. See `maraboupy/examples/7_PythonicAPI.py` for details.

## Version 1.0.0

* Initial versioned release

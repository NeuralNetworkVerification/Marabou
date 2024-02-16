# Changelog

## Next Release

* Dependency changes:
  - Dropped support for Python 3.7
  - Now use ONNX 1.15.0 (up from 0.12.0) in both C++ and Python backends.
  - The class `MarabouONNXNetwork` no longer depends on `torch` in Python backend.

* Marabou now prints errors on `stderr` rather than `stdout`

* Added proof producing versions of `Sign`, `Max`, `Absolute Value` and `Disjunction` constraints.

* Changes to command-line ONNX support:
  - Fixed bug with variable lower bounds not being set correctly.
  - Fixed bug with sigmoid operators not being parsed correctly.
  - Added support for `Tanh`, `Squeeze`, `LeakyRelu` and `Cast` operators.
  - Added support for networks with multiple outputs

* Added command-line support for properties in the VNNLIB format.

* Changes to Python ONNX support:
  - Added support for `Softmax`, `Bilinear` and `LeakyRelu` operators.
  - `MarabouONNXNetwork` no longer exposes the fields `madeGraphEquations`, `varMap`, `constantMap`, `shapeMap`
    as these were supposed to be internal implementation details.
  - `MarabouONNXNetwork` no longer has a `shallowCopy` method. Instead of calling this method,
    you should set the new parameter `preserveExistingConstraints` in the method `readONNX` to
    `True` which has the same effect.
  - The method `getMarabouQuery` on `MarabouNetwork` has been renamed `getInputQuery`.

* Added support for creating constraints using the overloaded syntax `<=`, `==` etc. in
  the Python backend. See `maraboupy/examples/7_PythonicAPI.py` for details.

## Version 1.0.0

* Initial versioned release

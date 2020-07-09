Maraboupy
=========

Maraboupy is the python interface for Marabou, an SMT-based 
neural network verification tool. 
Neural networks can be supplied in tensorflow, ONNX, or NNet formats,
and piecewise-linear constraints can be added to the network variables
to encode a desired property for verification. 
Maraboupy uses pybind11 to solve the neural network query using the C++
implementation of Marabou. In general, the solver will return either UNSAT if no
assignment of values to all network variables can satisfy all equations and
constraints, or SAT if a satisfying assignment exists.

This documentation explains how to setup Maraboupy, shows examples using Maraboupy,
and provides API documentation.


.. _Setup:
.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Setup

   Setup/*
   
.. _Examples:
.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Examples
   :includehidden:

   Examples/*Example*

.. _API:
.. toctree::
   :maxdepth: 1
   :glob:
   :caption: API Documentation

   API/*

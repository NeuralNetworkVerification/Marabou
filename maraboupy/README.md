#  Marabou

maraboupy - a python API for marabou.
### Requirements
- Python 3

### Installation

- Install PyBind11 for Python3: `pip3 install -U pybind11`
- Compile maraboupy:
- 		cd maraboupy 
 		make
- Add the root of the Marabou directory to your Python path 
-	one way to do it: `export PYTHONPATH=$PYTHONPATH:<Marabou root>`

### Usage
-		import maraboupy

The examples folder contains a sample Jupyter notebook, to demonstrate how to load a network and test various properties on it.

To use the Divide-and-conquer solver, both .nnet and .pb file must be available under the same directory with the same name.
For conversion between .nnet and .pb, refer to https://github.com/sisl/NNet/tree/master/scripts

### Examples
/examples/networks contains a set of example networks in .pb and .nnet format.

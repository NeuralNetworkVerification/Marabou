# CAV2019 Experiments

This folder contains files needed to reproduce Marabou experimental results from
the tool paper. 

## Neural networks
The neural networks benchmarks can be found in the nnet folder with a
folder for each category of benchmarks:

 - *acasxu*  contains the ACAS XU benchmarks from the Reluplex paper,
 - *coav*    contains the CollisionAvoidance benchmarks from the Planet paper,
 - *twin*    contains the TwinStreams benchmarks from the Planet paper.

## Properties
The property files can be found in the *properties* folder. There are 4
properties related to ACAS XU networks, named *acas_property_[1-4].txt* and one
property used for CollisionAvoidance and TwinStreams benchmarks, since the
property to be checked is are built-into the .nnet file

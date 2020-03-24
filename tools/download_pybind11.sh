#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

# TODO: add progress bar, -q is quite, if removing it the progress bar is in
# multiple lines
echo "downloading pybind"
wget -q -O pybind11_2_3_0.tar.gz https://github.com/pybind/pybind11/archive/v2.3.0.tar.gz

echo "unzipping pybind"
tar xzvf pybind11_2_3_0.tar.gz >> /dev/null

cd $curdir

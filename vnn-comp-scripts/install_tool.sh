
#! /usr/bin/env bash


script_name=$(realpath $0)
script_path=$(dirname "$script_name")
project_path=$(dirname "$script_path")
home=$HOME
export INSTALL_DIR="$home"
export GUROBI_HOME="$home/gurobi912/linux64"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
export GRB_LICENSE_FILE="$home/gurobi.lic"

if [[ ! -d $home/gurobi912/ ]]
then
    wget https://packages.gurobi.com/9.1/gurobi9.1.2_linux64.tar.gz
    tar xvfz gurobi9.1.2_linux64.tar.gz -C $INSTALL_DIR
fi

cd $INSTALL_DIR/gurobi912/linux64/src/build
make
cp libgurobi_c++.a ../../lib/

echo "Project dir:" $project_path
cd $project_path

pip3 install onnx onnxruntime
pip3 install onnx-simplifier

# install marabou
rm -rf build
mkdir build
cd build
cmake ../ -DENABLE_GUROBI=ON
make -j48
cd ../

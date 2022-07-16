#! /usr/bin/env bash

version=$1
benchmark=$2
onnx=$3
vnnlib=$4

pkill Marabou
pkill python

home=$HOME
export INSTALL_DIR="$home"
export GUROBI_HOME="$home/gurobi912/linux64"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
export GRB_LICENSE_FILE="$home/gurobi.lic"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

benchmark_dir=$(realpath "$SCRIPT_DIR"/benchmarks/)

if [[ ! -d $benchmark_dir ]]
then
    mkdir $benchmark_dir
fi

echo $benchmark
python3 -m onnxsim $onnx "$onnx"-simp
mv "$onnx"-simp $onnx


# uncompress onnx file if needed: "gzip -dk test_zipped.onnx.gz"
if [[ $vnnlib == *gz ]] # * is used for pattern matching
then
    UNCOMPRESSED_vnnlib=${vnnlib%.gz}

    if [ ! -f $UNCOMPRESSED_vnnlib ]
    then
        # UNCOMPRESSED_ONNX doesn't exist, create it
        echo "$UNCOMPRESSED_vnnlib doesn't exist, unzipping"
        gzip -vdk $vnnlib
    fi

    vnnlib=$UNCOMPRESSED_vnnlib
fi

"$SCRIPT_DIR"/../maraboupy/prepare_instance.py $onnx $vnnlib $benchmark_dir

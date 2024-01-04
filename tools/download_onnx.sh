#!/bin/bash
curdir=$pwd
mydir="${0%/*}"
onnxdir=onnx-1.12.0
protobufdir=protobuf-3.19.2

# This script downloads the protobuffer file from the Onnx repo.
# There are no C++ bindings for onnx so we compile the protobuffer ourselves.
# see https://stackoverflow.com/questions/67301475/parse-an-onnx-model-using-c-extract-layers-input-and-output-shape-from-an-on
# for details.

cd $mydir

mkdir -p $onnxdir
cd $onnxdir
echo "downloading onnx proto file"
wget -q https://raw.githubusercontent.com/onnx/onnx/v1.12.0/onnx/onnx.proto3 -O onnx.proto3

echo "compiling the onnx proto file"
../$protobufdir/installed/bin/protoc --cpp_out=. onnx.proto3

cd $curdir

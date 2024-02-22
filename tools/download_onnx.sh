#!/bin/bash
curdir=$pwd
mydir="${0%/*}"
version=$1
protobuf_version=$2

onnxdir=onnx-$version
protobufdir=protobuf-$protobuf_version

# This script downloads the protobuffer file from the ONNX repo.
# There are no C++ bindings for ONNX so we compile the protobuffer ourselves.
# see https://stackoverflow.com/questions/67301475/parse-an-onnx-model-using-c-extract-layers-input-and-output-shape-from-an-on
# for details.

cd $mydir

mkdir -p $onnxdir
cd $onnxdir
echo "Downloading ONNX proto file"
wget -q https://raw.githubusercontent.com/onnx/onnx/v$version/onnx/onnx.proto3 -O onnx.proto3

echo "Compiling the ONNX proto file"
../$protobufdir/installed/bin/protoc --cpp_out=. onnx.proto3

cd $curdir

#!/bin/bash

LIBTORCH_VERSION=$1
LIBTORCH_DIR=$(dirname $0)

TEMP_DIR=$(mktemp -d)
mkdir -p ${TEMP_DIR}

wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.3.1%2Bcpu.zip -O ${TEMP_DIR}/libtorch.zip
unzip -o ${TEMP_DIR}/libtorch.zip -d ${LIBTORCH_DIR}
rm ${TEMP_DIR}/libtorch.zip

rm -rf ${TEMP_DIR}

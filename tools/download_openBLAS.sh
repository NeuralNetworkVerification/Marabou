#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

echo "Downloading openBLAS"
wget -q https://github.com/xianyi/OpenBLAS/releases/download/v0.3.19/OpenBLAS-0.3.19.tar.gz -O OpenBLASv0.3.19.tar.gz
echo "Unzipping openBLAS"
tar -xzf OpenBLASv0.3.19.tar.gz >> /dev/null
echo "Installing openBLAS"
cd OpenBLAS-0.3.19

if [[ $OSTYPE == 'darwin'* ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi

make NO_SHARED=1 CBLAS_ONLY=1 USE_THREAD=1 >> /dev/null
mkdir installed/
make PREFIX=installed NO_SHARED=1 install >> /dev/null
cd $curdir

## TODO: open blas is in single thread mode. Is this what reluval uses?

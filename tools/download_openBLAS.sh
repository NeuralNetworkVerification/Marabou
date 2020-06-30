#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

echo "Downloding openBLAS"
wget -q http://github.com/xianyi/OpenBLAS/archive/v0.3.9.tar.gz -O OpenBLASv0.3.9.tar.gz
echo "Unzipping openBLAS"
tar -xzf OpenBLASv0.3.9.tar.gz >> /dev/null
echo "Installing openBLAS"
cd OpenBLAS-0.3.9
make NO_SHARED=1 CBLAS_ONLY=1 USE_THREAD=0 >> /dev/null
mkdir installed/
make PREFIX=installed NO_SHARED=1 install >> /dev/null
cd $curdir

## TODO: open blas is in single thread mode. Is this what reluval uses?

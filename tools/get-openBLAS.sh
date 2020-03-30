#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

echo "downloding openBLAS"
wget http://github.com/xianyi/OpenBLAS/archive/v0.3.9.tar.gz
echo "unzipping openBLAS"
tar -xzf v0.3.9.tar.gz >> /dev/null
echo "installing openBLAS"
cd OpenBLAS-0.3.9
make NO_SHARED=1 CBLAS_ONLY=1 USE_THREAD=0 >> /dev/null
mkdir installed/
make PREFIX=installed NO_SHARED=1 install >> /dev/null
cd $curdir

## TODO: open blas is in single thread mode. Is this what reluval uses?

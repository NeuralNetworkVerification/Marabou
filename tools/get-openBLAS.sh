#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

echo "downloding openBLAS"
wget http://github.com/xianyi/OpenBLAS/archive/v0.3.6.tar.gz
echo "unzipping openBLAS"
tar -xzf v0.3.6.tar.gz >> /dev/null
echo "installing openBLAS"
mv OpenBLAS-0.3.6/ OpenBLAS/
cd OpenBLAS/
make NO_SHARED=1 CBLAS_ONLY=1 USE_THREAD=0 -j4 >> /dev/null
mkdir installed/
make PREFIX=installed/ NO_SHARED=1 install >> /dev/null
cd $curdir

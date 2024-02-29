#!/bin/bash
curdir=$pwd
mydir="${0%/*}"
version=$1

cd $mydir

echo "Downloading PyTorch"
wget https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-$version%2Bcpu.zip -O libtorch-$version.zip -q --show-progress --progress=bar:force:noscroll

echo "Unzipping PyTorch"
unzip libtorch-$version.zip >> /dev/null
mv libtorch libtorch-$version

cd $curdir

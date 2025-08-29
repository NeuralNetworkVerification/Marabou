#!/bin/bash
curdir=$pwd
mydir="${0%/*}"
version=$1

cd $mydir

# Need to download the cxx11-abi version of libtorch in order to ensure compatability
# with boost.
#
# See https://discuss.pytorch.org/t/issues-linking-with-libtorch-c-11-abi/29510 for details.
echo "Downloading PyTorch"
wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-$version%2Bcpu.zip -O libtorch-$version.zip -q --show-progress --progress=bar:force:noscroll

echo "Unzipping PyTorch"
unzip libtorch-$version.zip >> /dev/null
mv libtorch libtorch-$version

cd $curdir

#!/bin/bash

get_abs_filename() {
  # $1 : relative filename
  echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

curdir=$pwd
mydir="${0%/*}"
installdir=$(get_abs_filename $mydir)/protobuf-3.19.2/installed

# Compatibility with MacOS
if [[ $OSTYPE == 'darwin'* ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi

cd $mydir
echo "downloading protobuf"
wget -q https://github.com/protocolbuffers/protobuf/releases/download/v3.19.2/protobuf-cpp-3.19.2.tar.gz -O protobuf-cpp-3.19.2.tar.gz
echo "unzipping protobuf"
tar -xzf protobuf-cpp-3.19.2.tar.gz # >> /dev/null
echo "installing protobuf"
cd protobuf-3.19.2
mkdir -p $installdir
./configure --disable-shared --prefix=$installdir --enable-fast-install
make -j16
make install

cd $curdir

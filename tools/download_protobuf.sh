#!/bin/bash

get_abs_filename() {
  # $1 : relative filename
  echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

curdir=$pwd
mydir="${0%/*}"
version=$1
installdir=$(get_abs_filename $mydir)/protobuf-$version/installed

# Compatibility with MacOS
if [[ $OSTYPE == 'darwin'* ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi

cd $mydir
echo "Downloading protobuf-$version"
wget -q https://github.com/protocolbuffers/protobuf/releases/download/v$version/protobuf-cpp-$version.tar.gz -O protobuf-cpp-$version.tar.gz

echo "Unzipping protobuf-$version"
tar -xzf protobuf-cpp-$version.tar.gz # >> /dev/null

echo "Installing protobuf-$version"
cd protobuf-$version
mkdir -p $installdir
./configure CXXFLAGS=-fPIC --disable-shared --prefix=$installdir --enable-fast-install
make
make install

cd $curdir

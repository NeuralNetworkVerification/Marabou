#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

# TODO: add progress bar, -q is quite, if removing it the progress bar is in
# multiple lines
echo "downloading boost"
wget -q https://sourceforge.net/projects/boost/files/boost/1.80.0/boost_1_80_0.tar.gz/download -O boost_1_80_0.tar.gz
echo "unzipping boost"
tar xzvf boost_1_80_0.tar.gz >> /dev/null
echo "installing boost"
cd boost_1_80_0;

if [[ $OSTYPE == 'darwin'* ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi

mkdir installed
./bootstrap.sh --prefix=`pwd`/installed --with-libraries=program_options,timer,chrono,thread >> /dev/null ;
./b2 cxxflags=-fPIC link=static cxxflags=-std=c++11 install >> /dev/null
mkdir installed32
./bootstrap.sh --prefix=`pwd`/installed32 --with-libraries=program_options,timer,chrono,thread >> /dev/null ;
./b2 cxxflags=-fPIC link=static cxxflags=-std=c++11 install address-model=32 >> /dev/null

cd $curdir

#!/bin/bash
curdir=$pwd
mydir="${0%/*}"
version=$1

cd $mydir

# TODO: add progress bar, -q is quite, if removing it the progress bar is in
# multiple lines
echo "Downloading boost"
underscore_version=${version//./_}
wget -q https://sourceforge.net/projects/boost/files/boost/$version/boost_$underscore_version.tar.gz/download -O boost-$version.tar.gz

echo "Unzipping boost"
tar xzvf boost-$version.tar.gz >> /dev/null

mv boost_$underscore_version boost-$version

echo "Installing boost"
cd boost-$version;
if [[ $OSTYPE == 'darwin'* ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi
mkdir installed
./bootstrap.sh --prefix=`pwd`/installed --with-libraries=program_options,timer,chrono,thread,regex >> /dev/null ;
./b2 cxxflags=-fPIC link=static cxxflags=-std=c++11 install >> /dev/null
mkdir installed32
./bootstrap.sh --prefix=`pwd`/installed32 --with-libraries=program_options,timer,chrono,thread,regex >> /dev/null ;
./b2 cxxflags=-fPIC link=static cxxflags=-std=c++11 install address-model=32 >> /dev/null

cd $curdir

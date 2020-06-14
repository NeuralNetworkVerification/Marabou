#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

# TODO: add progress bar, -q is quite, if removing it the progress bar is in
# multiple lines
echo "downloading boost"
wget -q https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
echo "unzipping boost"
tar xzvf boost_1_68_0.tar.gz >> /dev/null
echo "installing boost"
cd boost_1_68_0;
mkdir installed
./bootstrap.sh --prefix=`pwd`/installed --with-libraries=program_options >> /dev/null ;
./b2 cxxflags=-fPIC link=static install >> /dev/null
mkdir installed32
./bootstrap.sh --prefix=`pwd`/installed32 --with-libraries=program_options >> /dev/null ;
./b2 cxxflags=-fPIC link=static install address-model=32 >> /dev/null

cd $curdir

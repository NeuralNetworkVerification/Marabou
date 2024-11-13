#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir
cd cadical
echo "building cadical"
mkdir build
cd build
for f in ../src/*.cpp; do g++ -O3 -DNDEBUG -DNBUILD -DLOGGING -g -fPIC -c $f; done
ar rc libcadical.a `ls *.o | grep -v ical.o`
g++ -fPIC -o cadical cadical.o -L. -lcadical
g++ -fPIC -o mobical mobical.o -L. -lcadical
#./configure
#make

cd $curdir

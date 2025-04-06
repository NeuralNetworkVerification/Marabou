#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir
git clone https://github.com/arminbiere/cadical.git
cd cadical
git checkout d04d77b1ed04f08eebd78aeaff94673bd11330c8
mkdir build
cd build
for f in ../src/*.cpp; do g++ -O3 -DNDEBUG -DNBUILD -DLOGGING -g -fPIC -c $f; done
ar rc libcadical.a `ls *.o | grep -v ical.o`
g++ -fPIC -o cadical cadical.o -L. -lcadical
g++ -fPIC -o mobical mobical.o -L. -lcadical
#./configure
#make

cd $curdir

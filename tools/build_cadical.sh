#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

echo $mydir
cd $mydir
cd cadical
echo "building cadical"
./configure
make

cd $curdir

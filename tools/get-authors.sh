#!/bin/sh
#
# get-authors
# Copyright (c) 2017-2019, the Marabou project
#
# usage: get-authors [ files... ]
#
# This script uses git to get the original author
#

#
# This script was originally adapeted from a similar script in the
# CVC4 project: https://github.com/CVC4/CVC4
#

gituser="`git config user.name` <`git config user.email`>"

if [ "$1" = "--email" ]; then
  strip_email=cat
  shift
else
  strip_email="sed 's, *<[^>]*@[^>]*>,,g'"
fi

while [ $# -gt 0 ]; do
  f=$1
  shift
  contributors=
  if [ -z "`grep " \*\* Top contributors" $f`" ]
  then
    header_lines=0
  else
    header_lines=`grep "\*\*\/" $f -m 1 -n | cut -d ':' -f 1`
    if [ -z $header_lines ]; then header_lines=0; fi
  fi
  header_lines=$((header_lines+1))
#  ((header_lines++))
  total_lines=`wc -l "$f" | awk '{print$1}'`
  git blame -w -M --incremental -L $header_lines,$total_lines "$f" | \
    gawk '/^[0-9a-f]+ [0-9]+ [0-9]+ [0-9]+$/ {nl=$4;} /^summary .*copyright/ {nl=0} /^author / {$1=""; author=$0;} /^author-mail / {mail=$2} /^filename / {while(nl--) {print author,mail}}' | \
    sed "s,Not Committed Yet <not.committed.yet>,$gituser," | \
    sed 's/guykatzz/Guy/' | \
    sed 's/Guy Katz/Guy/' | \
    sed 's/Guy/Guy Katz/' | \
    sed 's/ibeling/Duligur Ibeling/' | \
    sed 's/rachellim/Rachel Lim/' | \
    sed 's/derekahuang/Derek Huang/' | \
    sed 's/ShantanuThakoor/Shantanu Thakoor/' | \
    sed 's/clazarus/Chris Lazarus/' | \
    sed 's/kjulian3/Kyle Julian/' | \
    sed 's/lkuper/Lindsey Kuper/' | \
    sed 's/anwu1219/Andrew Wu/' | \
    sed 's/sparth95/Parth Shah/' | \
    sed 's/jayanthkannan2/Jayanth Kannan/' | \
    sed 's/chelseas/Chelsea Sidrane/' | \
    sed 's/mfernan2/Matthew Fernandez/' | \
    eval "$strip_email" | \
    sort | uniq -c | sort -nr | head -n 3 | \
    ( while read lines author; do
        contributors="${contributors:+$contributors, }$author"
      done; \
      echo "$contributors")
done

#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SRC_DIR="$SCRIPT_DIR"/../src/

find $SRC_DIR -iname *.h -o -iname *.cpp | xargs clang-format -style=file -i
# Somehow for "src/input_parsers/AcasNnet.h" clang-format needs two passes to make it actually conforming.
find $SRC_DIR -iname *.h -o -iname *.cpp | xargs clang-format -style=file -i

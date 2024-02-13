#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SRC_DIR="$SCRIPT_DIR"/../src/

find $SRC_DIR -iname *.h -o -iname *.cpp | xargs clang-format -style=file -i

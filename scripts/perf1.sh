#!/usr/bin/env bash

set -ue

FILE="$(realpath -s "$1")"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

make release

valgrind --tool=callgrind --cache-sim=yes --dump-instr=yes --branch-sim=yes \
    bin/1brc $FILE


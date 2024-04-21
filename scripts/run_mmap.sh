#!/usr/bin/env bash

set -ue

FILE="$(realpath -s "$1")"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

make release

bin/1brc $FILE --mmap

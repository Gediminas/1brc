#!/usr/bin/env bash

set -ue

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

make clean

rm -f a.out 
rm -f perf.data*
rm -f callgrind*
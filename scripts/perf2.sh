#!/usr/bin/env bash

set -ue

FILE="$(realpath -s "$1")"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

make release-debug

perf record -e cycles,instructions,L1-dcache-loads,LLC-loads,LLC-load-misses,branches,branch-misses,cache-references,cache-misses --call-graph=dwarf \
  bin/1brc_rdebug $FILE

perf report -v

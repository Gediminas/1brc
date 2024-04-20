#!/usr/bin/env bash

set -ue

FILE="$(realpath -s "$1")"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

CMD="clear ;\
     make release &&\
     hyperfine --runs=1 --show-output \
         'bin/1brc $FILE --debug'"

find src/*.{c,h} | entr bash -c "$CMD"

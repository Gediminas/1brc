#!/usr/bin/env bash

set -ue

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

make all -k -j $(nproc)

echo "======================="
cd bin
pwd
ls -lah --color=always ./

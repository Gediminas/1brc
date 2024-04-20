#!/usr/bin/env bash

set -ue

FILE="$(realpath -s "$1")"
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

cd $ROOT

make release

$ROOT/scripts/clear_system_caches.sh

hyperfine --warmup=1 --runs=3 "$ROOT/bin/1brc $FILE"

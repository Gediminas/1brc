#!/usr/bin/env bash

echo 3 | sudo tee /proc/sys/vm/drop_caches

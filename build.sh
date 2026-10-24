#!/bin/bash

set -eu

cd $(dirname "$0")
mkdir -p build
gcc -o build/main src/main.c
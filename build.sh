#!/bin/bash

set -eu

cd $(dirname "$0")
mkdir -p build
rm --force build/main
gcc -o build/main -std=c99 -Wall -Wextra src/main.c
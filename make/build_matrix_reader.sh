#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common/ src/matrix/matrix_reader.c -o build_arm/matrix_reader

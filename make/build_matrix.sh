#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common/ src/matrix/matrixI2C.c -o build_arm/matrix

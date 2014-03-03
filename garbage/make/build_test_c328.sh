#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common/ src/common/c328.cpp -o build_arm/c328
bfin-uclinux-g++ -I src/common/ src/test/c328_read.cpp -o build_arm/c328_read

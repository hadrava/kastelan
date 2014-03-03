#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -D VERBOSE -I src/common/ src/common/i2c_communication.cpp src/common/variable_loader.cpp src/puck_sort/puck_sort.cpp src/common/cny.cpp -o build_arm/puck_sort

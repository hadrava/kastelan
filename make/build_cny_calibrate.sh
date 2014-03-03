#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common/ src/cny_calibrate/cny_calibrate.cpp src/common/variable_loader.cpp src/common/i2c_communication.cpp src/common/cny.cpp -o build_arm/cny_calibrate

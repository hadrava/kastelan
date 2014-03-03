#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common/ -I src/controller/ src/sequencer/main.cpp src/common/i2c_communication.cpp src/common/variable_loader.cpp src/controller/jiiizda.cpp src/common/check.cpp src/common/cny.cpp -o build_arm/main

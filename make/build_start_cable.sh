#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common/ src/start_cable/start_cable.cpp src/common/i2c_communication.cpp -o build_arm/start_cable

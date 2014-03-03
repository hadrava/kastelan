#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -D TEST_JIZDA -I src/controller/ -I src/common/  src/controller/jiiizda.cpp src/test/test_jiiizda/test_jiiizda.cpp src/common/variable_loader.cpp src/common/i2c_communication.cpp -o build_arm/jiiizda

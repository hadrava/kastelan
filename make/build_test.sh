#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -D TEST_AD                     src/common/i2c_communication.cpp -o build_arm/i2c_ad
bfin-uclinux-g++ -D TEST_SD20                   src/common/i2c_communication.cpp -o build_arm/i2c_sd20
bfin-uclinux-g++ -D TEST_GPIO                   src/common/i2c_communication.cpp -o build_arm/i2c_gpio
bfin-uclinux-g++ -D TEST_CNY src/common/cny.cpp src/common/i2c_communication.cpp -o build_arm/i2c_cny

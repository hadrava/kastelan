#!/bin/sh

cd ..

# Load config
. ../config
prepare_build_arm

g++ -I src/common \
    -o build_arm/main \
    -pthread \
    src/sequencer/main.cpp \
    src/common/led_driver.cpp \
    src/common/matrix_driver.cpp \
    src/common/i2c_communication.cpp \
    src/common/variable_loader.cpp \
    src/common/check.cpp \
    src/common/encoder_driver.cpp \
    src/common/our_time.cpp \
    src/common/motor_driver.cpp

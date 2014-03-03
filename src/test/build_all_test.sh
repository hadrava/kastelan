#!/bin/sh

cd ..

# Load config
. ../config
prepare_build_arm

g++ -I src/common \
    -o build_arm/all_test \
    -pthread \
    src/test/all_test.cpp \
    src/common/led_driver.cpp \
    src/common/input_event_driver.cpp \
    src/common/i2c_communication.cpp \
    src/common/variable_loader.cpp \
    src/common/check.cpp \
    src/common/encoder_driver.cpp \
    src/common/our_time.cpp \
    src/common/motor_driver.cpp \
# motor_driver.cpp je rozbity

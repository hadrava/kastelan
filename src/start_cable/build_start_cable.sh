#!/bin/sh

cd ..

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -I src/common \
    -o build_arm/start_cable \
    src/start_cable/start_cable.cpp \
    src/common/i2c_communication.cpp \
    src/common/variable_loader.cpp \


#     -pthread \
#     src/common/led_driver.cpp \
#     src/common/matrix_driver.cpp \
#     src/common/check.cpp \
#     src/common/encoder_driver.cpp \
#     src/common/our_time.cpp \
#     src/common/motor_driver.cpp \
# motor_driver.cpp je rozbity

#!/bin/sh

# Load config
. ../config
prepare_flash

sz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT" build_arm/i2c_ad build_arm/i2c_cny build_arm/i2c_gpio build_arm/i2c_sd20

#!/bin/sh

# Load config
. ../config
prepare_flash

sz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT" build_arm/jiiizda src/test/test_jiiizda/test_jiiizda.config

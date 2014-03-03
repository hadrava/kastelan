#!/bin/sh

# Load config
. ../config
prepare_flash

sz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT" build_arm/mem_alloc
sleep 0.1
sz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT" build_arm/mem_read
sleep 0.1
sz -y --tcp-client "$ROBOT_IP:$ROBOT_PORT" build_arm/mem_write

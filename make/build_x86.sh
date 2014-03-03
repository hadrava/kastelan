#!/bin/sh

# Load config
. ../config
prepare_build_x86

g++ -I src/common/ -I src/controller/ src/controller/jiiizda.cpp src/sequencer/main.cpp src/common/i2c_communication.cpp src/common/variable_loader.cpp src/common/check.cpp -o build_x86/state_machine

#!/bin/sh

# Load config
. ../config
prepare_build_x86

g++ -lopencv_core -lopencv_highgui -lopencv_imgproc src/test/c328_image_reader/viewer.cpp -o build_x86/c328_image_reader

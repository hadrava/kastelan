#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-gcc -I src/common/jpeg/ -O2 src/test/jpeg_webcam/jpeg_webcam_320_256.c src/common/jpeg/jpeg.c -x assembler-with-cpp src/common/jpeg/r8x8dct.asm -o build_arm/jpeg_webcam_320_256

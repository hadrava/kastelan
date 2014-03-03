#!/bin/sh

# Load config
. ../config
prepare_build_x86

g++ src/test/jpeg_webcam/jpeg_webcam_reader.c -o build_x86/jpeg_webcam_reader

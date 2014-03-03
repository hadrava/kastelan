#!/bin/sh

# Load config
. ../config
prepare_build_arm

bfin-uclinux-g++ -D MEM_ALLOC  src/test/memshare.cpp -o build_arm/mem_alloc
bfin-uclinux-g++ -D MEM_READ  src/test/memshare.cpp -o build_arm/mem_read
bfin-uclinux-g++ -D MEM_WRITE  src/test/memshare.cpp -o build_arm/mem_write

#!/bin/sh

for i in `ls flash_* | grep -v "flash_all.sh"`; do
  ./$i
done

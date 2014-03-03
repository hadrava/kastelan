#!/bin/sh

for i in `ls build_* | grep -v "build_all.sh"`; do
  ./$i
done

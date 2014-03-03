#!/bin/sh

echo srv1_encoder test script

modprobe srv1_encoder
mknod /dev/enc0 c 253 0
mknod /dev/enc1 c 253 1

cat /dev/enc0 > /mnt/enc00 & pid1=$!
cat /dev/enc1 > /mnt/enc10 & pid2=$!
sleep 5
kill $pid1 $pid2

echo > /dev/enc0& pid1=$!
echo > /dev/enc1& pid2=$!
sleep 5
kill $pid1 $pid2

cat /dev/enc0 > /mnt/enc01 & pid1=$!
cat /dev/enc1 > /mnt/enc11 & pid2=$!
sleep 5
kill $pid1 $pid2

echo 1 > /dev/enc0& pid1=$!
echo 1 > /dev/enc1& pid2=$!
sleep 5
kill $pid1 $pid2

cat /dev/enc0 > /mnt/enc02 & pid1=$!
cat /dev/enc1 > /mnt/enc12 & pid2=$!
sleep 5
kill $pid1 $pid2

echo -n 1 > /dev/enc0& pid1=$!
echo -n 1 > /dev/enc1& pid2=$!
sleep 5
kill $pid1 $pid2

cat /dev/enc0 > /mnt/enc03 & pid1=$!
cat /dev/enc1 > /mnt/enc13 & pid2=$!
sleep 5
kill $pid1 $pid2

sync

echo R > /dev/enc0& pid1=$!
echo R > /dev/enc1& pid2=$!
sleep 5
kill $pid1 $pid2

cat /dev/enc0 > /mnt/enc04 & pid1=$!
cat /dev/enc1 > /mnt/enc14 & pid2=$!
sleep 5
kill $pid1 $pid2

echo D > /dev/enc0& pid1=$!
echo D > /dev/enc1& pid2=$!
sleep 5
kill $pid1 $pid2

cat /dev/enc0 > /mnt/enc05 & pid1=$!
cat /dev/enc1 > /mnt/enc15 & pid2=$!
sleep 5
kill $pid1 $pid2

sync

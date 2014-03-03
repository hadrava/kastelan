#!/bin/sh

LANG=""
export LANG

echo Zadejte verzi:
read revision
mkdir $revision
cd $revision

cd srv1-linux-read-only

PATH=$PATH:/opt/uClinux/bfin-uclinux/bin/:/opt/uClinux/bfin-linux-uclibc/bin/

if [ -x ../../source/patch.sh ]; then
  echo pridavam vlastni patche
  ../../source/patch.sh
fi

echo Kompiluji
make build

cp uClinux-dist/images/uImage.initramfs uImage.initramfs


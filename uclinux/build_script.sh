#!/bin/sh

LANG=""
export LANG

echo Zadejte verzi:
read revision
mkdir $revision
cd $revision

if [ -e ../source/srv1-linux-read-only.tar.gz ]; then
  echo Nepouzivam originalni svn
  echo Nevolam "svn checkout http://srv1-linux.googlecode.com/svn/trunk/ srv1-linux-read-only"
  echo Pouzivam ../source/srv1-linux-read-only.tar.gz
  ln -s ../source/srv1-linux-read-only.tar.gz srv1-linux-read-only.tar.gz
  tar -xvf srv1-linux-read-only.tar.gz
else
  echo Pouzivam originalni svn
  svn checkout http://srv1-linux.googlecode.com/svn/trunk/ srv1-linux-read-only
fi

cd srv1-linux-read-only

[ -e ../../source/uClinux-dist-2009R1.1-RC4.tar.bz2 ] || ../../source/download_uclinux.sh

if [ -e ../../source/uClinux-dist-2009R1.1-RC4.tar.bz2 ]; then
  echo Pouzivam jiz stazeny balik uClinux-dist-2009R1.1-RC4.tar.bz2
  ln -s ../../source/uClinux-dist-2009R1.1-RC4.tar.bz2 uClinux-dist-2009R1.1-RC4.tar.bz2
  cat Makefile  | sed 's/wget/echo Nestahuji/' > Makefile.new
  mv Makefile.new Makefile
fi

echo Spoustim make init
make init
echo Aplikuji zmeny v srv1 repozitari z uClinux do uClinux-dist
cp uClinux/* uClinux-dist/ -r
rm uClinux -rf
ln -s uClinux-dist uClinux

echo Nevolam defaultni make config menuconfig
echo Pouzivam standartni make -C uClinux config
echo Vybiram SRV1
echo 62 | make -C uClinux config

PATH=$PATH:/opt/uClinux/bfin-uclinux/bin/:/opt/uClinux/bfin-linux-uclibc/bin/

cat uClinux/config/.config \
| sed s/CONFIG_USER_MTDUTILS=y/CONFIG_USER_MTDUTILS=n/ \
| sed s/CONFIG_USER_SETSERIAL_SETSERIAL=y/CONFIG_USER_SETSERIAL_SETSERIAL=n/ \
| sed s/CONFIG_USER_NANO=y/CONFIG_USER_NANO=n/ \
> .config.new
mv .config.new uClinux/config/.config

echo Nyni je konfigurace funkcni

if [ -x ../../source/patch.sh ]; then
  echo pridavam vlastni patche
  ../../source/patch.sh
fi

echo Kompiluji
make build

cp uClinux-dist/images/uImage.initramfs uImage.initramfs


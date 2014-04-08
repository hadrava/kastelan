#!/bin/bash

list="`find logs/g1/ logs/final/ -type f`"

make gui_image
for i in $list; do
	mkdir -p tmp/$i
	pwd="`pwd`"
	cd tmp/$i
	"${pwd}/gui_image" < "${pwd}/$i"
	cd "$pwd"
done

mkdir -p out
for i in $list; do
	pwd="`pwd`"
	cd tmp/$i
	"${pwd}/convert_to_video.sh"
	cd "$pwd"
	j=`echo "$i" | tr "/" "_"`
	mv tmp/${i}/out.mkv out/${j}.mkv
done

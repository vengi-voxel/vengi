#!/bin/sh

APP=$1
ICON=$2

mkdir -p $APP.iconset

for i in 16 32 64 128 256 512 ; do
    sips -z $i $i $ICON --out $APP.iconset/icon_${i}x${i}.png
    sips -z $i $i $ICON --out $APP.iconset/icon_$((i/2))x$((i/2))x2.png
done

iconutil -c icns -o $APP.icns $APP.iconset


# ./icons.sh vengi-voxedit ../../../data/voxedit/voxedit-icon-665x665.png

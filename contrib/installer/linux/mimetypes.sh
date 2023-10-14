#!/bin/bash

SELF=$(readlink -f "${0}")
DIRECTORY=$(dirname "$SELF")
cd $DIRECTORY

mimetypes=$(xmlstarlet sel -N x="http://www.freedesktop.org/standards/shared-mime-info" -t -m "//x:mime-type/@type" -v . -n x-voxel.xml | sort)
#mimetypes=$(xmllint --xpath "//*[local-name()='mime-type']/@type" x-voxel.xml | awk -F\" '{ print $2 }' | sort)
mimetypes=$(echo $mimetypes | sed 's/ /;/g')

sed -i "s#MimeType=.*#MimeType=$mimetypes#g" voxedit.desktop.in
sed -i "s#MimeType=.*#MimeType=$mimetypes#g" thumbnailer.thumbnailer.in

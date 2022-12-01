#!/bin/bash

mimetypes=$(xmllint --xpath "//*[local-name()='mime-type']/@type" x-voxel.xml | awk -F\" '{ print $2 }' | sort)
mimetypes=$(echo $mimetypes | sed 's/ /;/g')

sed -i "s#MimeType=.*#MimeType=$mimetypes#g" voxedit.desktop.in
sed -i "s#MimeType=.*#MimeType=$mimetypes#g" thumbnailer.thumbnailer.in

#!/bin/bash

SELF=$(readlink -f "${0}")
DIRECTORY=$(dirname "$SELF")
cd $DIRECTORY

# TODO: CFBundleDocumentTypes and UTExportedTypeDeclarations
echo "    <key>CFBundleTypeExtensions</key>"
echo "    <array>"
for p in $(xmlstarlet sel -N x="http://www.freedesktop.org/standards/shared-mime-info" -t -m "//x:glob/@pattern" -v . -n ../linux/x-voxel.xml | sort); do
	p=$(echo $p | sed 's/*.//g')
	echo "        <string>$p</string>"
done
echo "    </array>"

echo "    <key>CFBundleTypeMIMETypes</key>"
echo "    <array>"
for p in $(xmlstarlet sel -N x="http://www.freedesktop.org/standards/shared-mime-info" -t -m "//x:mime-type/@type" -v . -n ../linux/x-voxel.xml | sort); do
	echo "        <string>$p</string>"
done
echo "    </array>"

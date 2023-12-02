#!/bin/bash

SELF=$(readlink -f "${0}")
DIRECTORY=$(dirname "$SELF")
cd $DIRECTORY

# TODO: CFBundleDocumentTypes and UTExportedTypeDeclarations

xmlstarlet ed -L -d "//key[text()='CFBundleTypeExtensions']/following-sibling::array/*" application.plist.in
for p in $(xmlstarlet sel -N x="http://www.freedesktop.org/standards/shared-mime-info" -t -m "//x:glob/@pattern" -v . -n ../linux/x-voxel.xml | sed 's/*.//g' | sort | uniq); do
	xmlstarlet ed -L -s "//key[text()='CFBundleTypeExtensions']/following-sibling::array" -t elem -n string -v "$p" application.plist.in
done

# xmlstarlet ed -L -d "//key[text()='CFBundleTypeMIMETypes']/following-sibling::array/*" application.plist.in
# for p in $(xmlstarlet sel -N x="http://www.freedesktop.org/standards/shared-mime-info" -t -m "//x:mime-type/@type" -v . -n ../linux/x-voxel.xml | sort); do
# 	xmlstarlet ed -L -s "//key[text()='CFBundleTypeMIMETypes']/following-sibling::array" -t elem -n string -v "$p" application.plist.in
# done

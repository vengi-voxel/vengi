# About

![image](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/thumbnailer.jpg)

This application needs an opengl context. It is a command line tool running headless (meaning you don't see a window popping up).

## Linux Filemanagers

Create thumbnailer images of all supported voxel formats. In combination with a mimetype definition and a `.thumbnailer` definition file
that must be installed in `/usr/share/mime/packages` and `/usr/share/thumbnailer` this will e.g. create small preview images for the [supported voxel formats](../Formats.md).

It works for any file manager that supports `.thumbnailer` entries, including Nautilus, Thunar (when tumbler is installed), Nemo, Caja,
and PCManFM.

## Windows Explorer

There is a `voxelthumb.dll` that must get registered via `regsvr32 /s voxelthumb.dll`.

> You can still run this application from the windows command line to generate thumbnail images of your voxel models. See the [examples](Examples.md) for more details.

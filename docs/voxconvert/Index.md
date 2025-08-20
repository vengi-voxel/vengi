# About

Command line tool (and UI variant) to convert voxel volume, image or polygon formats between each other.

Modifying the voxels is also supported - rotating, translating, create lod, extract palette, execute lua scripts and a lot more.

If you need a way to automate your voxel assets for your art pipeline, `vengi-voxconvert` might be a tool you find useful.

If you e.g. need to apply a snow layer on top of your voxels, put grass everywhere or just thicken your voxels - check our the [lua script](../LUAScript.md) integration and the default scripts that are shipped with the tool.

If you have images that you would like to convert to voxels including depth, you can import the image as a plane and apply the depth values for each voxel.

Just check out the examples and the usage links below.

If you dislike the tool, found a [bug](https://github.com/vengi-voxel/vengi/issues) or need a [feature](https://github.com/vengi-voxel/vengi/issues), please let us know.

# Windows

**How to run the `vengi-voxconvert` command line tool on Windows:**

Open a terminal and navigate to the folder where you extracted the files. You can use `cmd.exe` or `PowerShell`.

To run the tool, use the command `vengi-voxconvert.exe` followed by the parameters you want to use.

# OSX

Open the dmg and go e.g. to `vengi-voxconvert.app` - use **Show Package Contents** to see the binaries in the app. Go to `Contents/MacOS` and copy the contained files into a new folder.

Use Shift+Open for running the cli - otherwise it will exit immediately.

# Further reading

* [Usage](Usage.md)
* [Configuration](Configuration.md)
* [Examples](Examples.md)
* [Screenshots](Screenshots.md)
* [Supported formats](../Formats.md)
* [Scripting support](../LUAScript.md)
* [Palettes](../Palette.md)

# Examples

[![asciicast](https://asciinema.org/a/fbcP4hsWvLXYnLyDbkwhEGQaH.svg)](https://asciinema.org/a/fbcP4hsWvLXYnLyDbkwhEGQaH)

Make sure to check out the [configuration](Configuration.md) section to see cvars that can influence the result.

## Animate a character

`animate` is a [lua script](../LUAScript.md).

Animate a given scene if the nodes are named correctly.

Valid names are `belt`, `head`, `foot`, `shoulder` or `hand` with either *left* or *right* indicators.

e.g. `belt`, `belt_left`, `belt_right`, `left_belt`, `right_belt`, `belt_l`, `belt_r`, `l_belt`, `r_belt`

Model should use a right-handed system - this basically means it should look into the negative z direction (and right shoulder should be along the positive x axis, y is up).

The model should have the correct parent and child relationships (a hand or arm is a child of a shoulder, a foot is child of a leg, etc).

`./vengi-voxconvert --script "animate all" --input character.vengi --output character_animated.vengi`

> You can extend that script to transfer your own animations to all your characters.

## Generate a map with lua

There are several map generators available.

`./vengi-voxconvert --script aos_forest_river --output aos_forest_river.vengi`

They are generating for the Ace of Spades map size (based on the public domain code from [aoemap](https://silverspaceship.com/aosmap/) - so we can also directly create the `vxl` for Ace of Spades.

`./vengi-voxconvert --script aos_forest_river --output aos_forest_river.vxl`

## Level of detail (LOD)

Generate a lod scaled by 50% from the input model.

`./vengi-voxconvert --scale --input infile.vox --output output.vox`

## Convert to multiple different formats

Convert the model into multiple different formats in the same run

`./vengi-voxconvert --input infile.vox --output output.gox --output output.cub`

## Import 2d image as volume

Imports a 2d image and applies depth to it.

`./vengi-voxconvert -set voxformat_imageimporttype 2 -set voxformat_imagevolumemaxdepth 8 -set voxformat_imagevolumebothsides true --input infile.png --output output.vox`

Import given input image as volume. Uses a depth map to make a volume out of the image. The depth map R channel is using values from 0 (black) to white (255) resulting in voxel heights from 1 to max-height (see `voxformat_imagevolumemaxdepth`).

The `--input` with e.g. `infile.png` will pick the depth map next to the image path called `infile-dm.png` as depth map.

> There is also a [lua script](../LUAScript.md) available called `imageasvolume` that can do the same.

There are other image import types available:

* `voxformat_imageimporttype 0` - import planes/slices in the z direction - or just as plane if not more than one slice was found
* `voxformat_imageimporttype 1` - import as heightmap (top view) with color in rgb and height in red channel - also see `voxformat_imageheightmapminheight` - if the color components are not 4 or the image is grayscale, we just import with a fixed color.
* `voxformat_imageimporttype 2` - import as volume with depth map (default) - also see `voxformat_imagevolumemaxdepth` and `voxformat_imagevolumebothsides`

## Slice a volume into png images

`./vengi-voxconvert --input yourfile.vox --merge --output output.png -set voxformat_imagesavetype 0`

This imports `yourfile.vox` - merges all the nodes into one and then export the png slices.

There are other png save options available

* `voxformat_imagesavetype 0` - planes/slices in the z direction
* `voxformat_imagesavetype 1` - export as heightmap (top view) with color in rgb and height in alpha channel
* `voxformat_imagesavetype 3` - thumbnail view (this is producing different results between `vengi-voxconvert` and `vengi-voxedit`)

## Generate from heightmap

Just specify the heightmap as input file like this:

`./vengi-voxconvert --input heightmap.png --output outfile.vox -set voxformat_imageimporttype 1`

It's assumed that the given image is a gray scale image - but only the red channel is used anyway.

If you want to colorize the surface of your heightmap import, you can just provide a heightmap that is no grayscale and the height is taken from the alpha channel of the image - the rgb channels of the image to determine the surface color.

## Convert all obj files in a zip

`./vengi-voxconvert --input input.zip --wildcard "*.obj" --output output.vengi`

## Replace the colors with a different palette

`replacepalette` is a [lua script](../LUAScript.md) that is able to replace or remap the colors of an existing palette to a new palette. You can specify the [built-in palettes](../Palette.md) or filenames to supported [palette formats](../Formats.md).

`./vengi-voxconvert --input input.vox --script "replacepalette built-in:minecraft true" --output mincraft.vox`

## Export flat normal gltf

If you want to export flat normals for your `gltf/glb` file, you can use disable to re-use existing vertices and duplicate the vertices. This gives you flat normals for each vertex.

`./vengi-voxconvert -set voxformat_reusevertices false --input input.vengi --output output.gltf`

By setting `voxformat_reusevertices` to `true` you get surface normals for your mesh.

> **NOTE:** This only works for cubic voxels - if you use another meshing algorithm (see [voxformat_meshmode](../Configuration.md) cvar), this setting most likely doesn't have the same effect.

## Export all nodes as single files

Save all nodes in the `input.vengi` file as dedicated files named after the names of the nodes.

`./vengi-voxconvert --export-models --input input.vengi`

Same as above, but change the target format to `kv6` in this example

`./vengi-voxconvert --export-models --input input.vengi --output output.kv6`

> Please keep in mind that the target format must be able to save the particular nodes of the source format. There might be restrictions on dimensions. They are not automatically split. See the other available options regarding splitting of nodes.

## Merge several models

Merge several models into one:

`./vengi-voxconvert --input one.vox --input two.vox --output onetwo.vox`

## Split objects into single volumes

Split voxels with the palette index `66` into own nodes.

`./vengi-voxconvert --script splitcolor --scriptcolor 66 --input infile.vox --output outfile.vox`

Splitting single objects that are not connected to other objects can also be split into own nodes.

`./vengi-voxconvert --script splitobject --input infile.vox --output outfile.vox`

Slice the model into smaller pieces (`width:height:depth`).

`./vengi-voxconvert --split 10:10:10 --input infile.vox --output outfile.vox`

## Handle a ply point cloud import

A `ply` file without face definitions is handled as point cloud. You can use the `voxformat_pointcloudsize` cvar (see [configuration](../Configuration.md)) to specify the size of the voxels and to connect them.

`./vengi-voxconvert --input infile_pointcloud.ply --output outfile.vox`

Of course it's also possible to convert the point cloud directly into a mesh

`./vengi-voxconvert --input infile_pointcloud.ply --output outfile.obj`

And you can also combine this with marching cubes meshing.

## Voxelize a obj, gltf, ply, fbx, stl or any supported mesh file

Voxelize an obj and save as magicavoxel (including colors):

> You are getting better results if you create a palette from your textures first.
>
> This can be done by reducing the color to 256 and import that reduced image in
> [voxedit](../voxedit/Index.md) as palette.
>
> For obj the mtl file must be in the same dir as the obj files, as well as the
> potential textures.
>
> glTF is supported both in ascii and binary - but the satellite files must also
> reside in the same dir as the `glb` or `gltf` file.

`./vengi-voxconvert -set voxformat_scale 2 -set palette /path/to/palette.png -set voxformat_createpalette false --input mesh.obj --output voxels.vox`

> See the [supported formats](../Formats.md) for a few more details.

If you also need normals you can specify the used palette with the cvar `normalpalette` - similar to the `palette` cvar given in the example above.

## Rotate the voxels

You can rotate the voxels around the x axis by 180 degree like this:

`./vengi-voxconvert --rotate x:180 --input infile.vox --output outfile.vox`

This of course also works for `y` and `z`, too.

## Translate the voxels

You can translate the voxels in the world like this:

`./vengi-voxconvert --translate 0:10:0 --input heightmap.png --output outfile.vox`

This would move the voxels 10 units upwards. But keep in mind that not every format supports
to store a translation offset.

## Execute lua script

Use the `--script` parameter:

`./vengi-voxconvert --script "cover 2" --input infile.vox --output outfile.vox`

This is executing the script in `./scripts/cover.lua` with a parameter of `2`.

`./vengi-voxconvert --script "./scripts/cover.lua 2" --input infile.vox --output outfile.vox`

This is doing exactly the same as above - just with a full path.

It's also possible to execute a lua script that is generating the voxels. Without any input file.

`./vengi-voxconvert --script aos_rainbow_towers --output outfile.vox`

See the [scripting](../LUAScript.md) documentation for further details.

## Extract palette png

Saves the png in the same dir as the vox file:

`./vengi-voxconvert --export-palette --input infile.vox --output outfile.vox`

There will be an `infile.png` now.

## Convert to a different palette

Convert the input file colors to a new palette by selecting the closest match:

`./vengi-voxconvert -set palette built-in:nippon -set voxformat_createpalette false --input infile.vox --output outfile.vox`

## Extract single models

Extract just a few models from the input file.

`./vengi-voxconvert --filter 1-2,4 --input infile.vox --output outfile.vox`

This will export models 1, 2 and 4.

Another option is to filter by node properties:

`./vengi-voxconvert --filter-property propertyName:propertyValue --input infile.vox --output outfile.vox`

Or by just checking whether a property is set - no matter which value:

`./vengi-voxconvert --filter-property propertyName --input infile.vox --output outfile.vox`

So every model node that has the node property `propertyName` set will get exported into `outfile.vox`.

You can find all node properties by using `--json`:

`./vengi-voxconvert --json --input infile.vox`

If you want the vertex details per node and for the whole scene, you should use `--json full`.

## Convert to mesh

You can export your volume model into a gltf, obj, stl or ply (see [Formats](../Formats.md) for more options)

`./vengi-voxconvert --input infile.vox --output outfile.obj`

`./vengi-voxconvert --input infile.vox --output outfile.gltf -set voxformat_transform_mesh true`

## Convert to different formats

### Convert qubicle to cubzh

`./vengi-voxconvert --input infile.qb --output output.3zh`

### Convert minecraft to magicavoxel

`./vengi-voxconvert --input infile.mca --output output.vox`

### Convert sandbox voxedit to qubicle

`./vengi-voxconvert --input infile.vxm --output output.qb`

## Batch convert

To convert a complete directory of e.g. `*.vox` to `*.obj` files, you can use e.g. the bash like this:

### Bash (Linux, OSX)

> Beware - no whitespaces in files

```bash
for i in *.vox; do vengi-voxconvert --input "$i" --output "${i%.vox}.obj"; done
```

```bash
for i in *.vxm; do vengi-voxconvert --input "$i" --output "${i%.vxm}.gltf" done
```

### PowerShell (Windows)

> https://docs.microsoft.com/en-us/powershell/scripting/learn/ps101/06-flow-control?view=powershell-7.2

An example for the windows powershell to extract single models into a new model

```ps
$array = "1-2,5", "1-2,7"
foreach ($i in $array){
  & .\vengi-voxconvert --filter $i --input "input.vox" --output "output_$i.vxm"
}
```

> https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.management/get-childitem

Convert all magicavoxel vox files in the current directory into gltf files

```ps
foreach ($file in Get-ChildItem -Filter "*.vox") {
   & .\vengi-voxconvert.exe --input "$($file.FullName)" --output "$($file.BaseName).gltf"
}
```

### Batch file (Windows)

> https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/for

Windows batch (or `cmd.exe`) example to convert all png in the current directory into magicavoxel vox files.

```cmd
for %f in (*.png) do call vengi-voxconvert --input "%~f" --output "%~nf.vox"
```

Process all pngs in the `Input` folder with a depth map to create voxel models in the `Output` folder.

```cmd
@echo off
setlocal enabledelayedexpansion

set INPUT_FOLDER=\Input
set OUTPUT_FOLDER=\Output

for %%f in (%INPUT_FOLDER%*.png) do (
    set "input_file=%%f"
    set "file_name=%%~nf"

    echo Processing !input_file!
    if not "!file_name!"=="!file_name:-dm=!" (
        echo Skipping !input_file! due to -dm in the name.
    ) else (
        set "output_file=%OUTPUT_FOLDER%!file_name!.vox"
        vengi-voxconvert -set voxformat_imageimporttype 2 -set voxformat_imagevolumemaxdepth 2 -set voxformat_imagevolumebothsides false --input "!input_file!" --output "!output_file!"
    )
)
```

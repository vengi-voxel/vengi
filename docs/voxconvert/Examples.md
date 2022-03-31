# Examples

[![asciicast](https://asciinema.org/a/fbcP4hsWvLXYnLyDbkwhEGQaH.svg)](https://asciinema.org/a/fbcP4hsWvLXYnLyDbkwhEGQaH)

## Level of detail (LOD)

Generate a lod scaled by 50% from the input model.

`./vengi-voxconvert -s --input infile.vox --output output.vox`

## Import 2d image as volume

Imports a 2d image and applies depth to it.

`./vengi-voxconvert --image-as-volume --image-as-volume-max-depth 8 --image-as-volume-both-sides true --input infile.png --output output.vox`

Import given input image as volume. Uses a depth map to make a volume out of the image. The depth map R channel is using values from 0 (black) to white (255) resulting in voxel heights from 1 to max-height (see `--image-as-volume-max-depth`).

The `--input` with e.g. `infile.png` will pick the depth map next to the image path called `infile-dm.png` as depth map.

## Merge several models

Merge several models into one:

`./vengi-voxconvert --input one.vox --input two.vox --output onetwo.vox`

## Voxelize an obj

Voxelize an obj and save as magicavoxel (including colors):

> You are getting better results if you create a palette from your textures first.
>
> This can be done by reducing the color to 256 and import that reduced image in
> [voxedit](../voxedit/Index.md) as palette.
>
> This only works for obj files - but if you have different mesh formats that you
> would like to voxelize, you can use the [assimp tools](https://github.com/assimp/assimp) to
> convert to obj.

`./vengi-voxconvert -set palette /path/to/palette.png --input mesh.obj --output voxels.vox`

## Generate from heightmap

Just specify the heightmap as input file like this:

`./vengi-voxconvert --input heightmap.png --output outfile.vox`

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

See the [scripting](../LUAScript.md) documentation for further details.

## Extract palette png

Saves the png in the same dir as the vox file:

`./vengi-voxconvert --src-palette --export-palette --input infile.vox --output outfile.vox`

There will be an `infile.png` now.

## Extract single layers

Extract just a few layers from the input file.

`./vengi-voxconvert --filter 1-2,4 --input infile.vox --output outfile.vox`

This will export layers 1, 2 and 4.

## Convert to mesh

You can export your volume model into a obj or ply.

`./vengi-voxconvert --input infile.vox --output outfile.obj`

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

An example for the windows powershell to extract single layers into a new model

```ps
$array = "1-2,5", "1-2,7"
foreach ($i in $array){
  ./vengi-voxconvert --filter $i --input "input.vox" --output "output_$i.vxm"
}
```

### Batch file (Windows)

> https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/for

Windows batch (or `cmd.exe`) example to convert all png in the current directory into magicavoxel vox files.

```cmd
for %f in (*.png) do call vengi-voxconvert --input "%~f" --output "%~nf.vox"
```

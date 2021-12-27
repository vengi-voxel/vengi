# Examples

## Level of detail (LOD)

Generate a lod scaled by 50% from the input model.

`./vengi-voxconvert -s --input infile.vox output.vox`

## Merge several models

Merge several models into one:

`./vengi-voxconvert --input one.vox --input two.vox --output onetwo.vox`

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

```bash
for i in *.vox; do vengi-voxconvert --input $i --output ${i%.vox}.obj; done
```

An example for the windows powershell to extract single layers into a new model

```ps
$array = "1-2,5", "1-2,7"
foreach ($i in $array){
  ./vengi-voxconvert --filter $i --input input.vox --output output_$i.vxm
}
```

# General

Convert voxel volume formats between each other or export to obj or ply.

* [Supported voxel formats](../Formats.md)
* [Scripting support](../LUAScript.md)

## Usage

`./vengi-voxconvert --merge --scale infile outfile`

* `--export-palette`: will save the included palette as png next to the source file. Use in combination with `--src-palette`.
* `--filter`: will filter out layers not mentioned in the expression. E.g. `1-2,4` will handle layer 1, 2 and 4. It is the same as `1,2,4`. The first layer is `0`.
* `--force`: overwrite existing files
* `--merge`: will merge a multi layer volume (like vox, qb or qbt) into a single volume of the target file
* `--mirror`: allows you to mirror the volumes at x, y and z axis
* `--rotate`: allows you to rotate the volumes by 90 degree at x, y and z axis
* `--scale`: perform lod conversion of the input volume (50% scale per call)
* `--script`: execute the given script - see [scripting support](../LUAScript.md) for more details
* `--src-palette`: will use the included palette and doesn't perform any quantization to the default palette

Just type `vengi-voxconvert` to get a full list of commands and options.

Using a different target palette is also possible by setting the `palette` config var.

`./vengi-voxconvert -set palette /path/to/palette.png infile outfile`

The palette file has to be in the dimensions 1x256. It is also possible to just provide the basename of the palette.
This is e.g. `nippon`. The tool will then try to look up the file `palette-nippon.png` in the file search paths.

You can convert to a different palette with this command. The closest possible color will be chosen for each
color from the source file palette to the specified palette.

## Level of detail (LOD)

Generate a lod scaled by 50% from the input model.

`./vengi-voxconvert -s infile.vox lod1.vox`

## Generate from heightmap

Just specify the heightmap as input file like this:

`./vengi-voxconvert heightmap.png outfile.vox`

## Execute lua script

Use the `--script` parameter:

`./vengi-voxconvert --script "cover 2" infile.vox outfile.vox`

This is executing the script in `./scripts/cover.lua` with a parameter of `2`.

`./vengi-voxconvert --script "./scripts/cover.lua 2" infile.vox outfile.vox`

This is doing exactly the same as above - just with a full path.

See the [scripting](../LUAScript.md) documentation for further details.

## Extract palette png

Saves the png in the same dir as the vox file

`./vengi-voxconvert --src-palette --export-palette infile.vox outfile.vox`

There will be an `infile.png` now.

## Extract single layers

Extract just a few layers from the input file.

`./vengi-voxconvert --filter 1-2,4 infile.vox outfile.vox`

This will export layers 1, 2 and 4.

## Convert to mesh

You can export your volume model into a obj or ply.

`./vengi-voxconvert infile.vox outfile.obj`

Config vars to control the meshing:

* `voxformat_ambientocclusion`: Don't export extra quads for ambient occlusion voxels
* `voxformat_mergequads`: Merge similar quads to optimize the mesh
* `voxformat_reusevertices`: Reuse vertices or always create new ones
* `voxformat_scale`: Scale the vertices by the given factor
* `voxformat_quads`: Export to quads
* `voxformat_withcolor`: Export vertex colors
* `voxformat_withtexcoords`: Export texture coordinates

See `./vengi-voxconvert --help` or [configuration](../Configuration.md) for more details.

## Batch convert

To convert a complete directory of e.g. `*.vox` to `*.obj` files, you can use e.g. the bash like this:

```bash
for i in *.vox; do vengi-voxconvert $i ${i%.vox}.obj; done
```

An example for the windows powershell to extract single layers into a new model

```ps
$array = "1-2,5", "1-2,7"
foreach ($i in $array){
  ./vengi-voxconvert --filter $i input.vox output_$i.vxm
}
```

## Screenshots

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxconvert-export-to-obj.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxconvert-export-obj.png)

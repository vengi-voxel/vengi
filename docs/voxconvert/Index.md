# General

Convert voxel volume formats between each other or export to obj or ply.

[Supported voxel formats](../Formats.md)

## Usage

`./vengi-voxconvert --merge --scale infile outfile`

* `--merge`: will merge a multi layer volume (like vox, qb or qbt) into a single volume of the target file
* `--scale`: perform lod conversion of the input volume (50% scale per call)

Just type `vengi-voxconvert` to get a full list of commands and options.

Using a different target palette is also possible by setting the `palette` config var.

`./vengi-voxconvert -set palette /path/to/palette.png infile outfile`

The palette file has to be in the dimensions 1x256. It is also possible to just provide the basename of the palette.
This is e.g. `nippon`. The tool will then try to look up the file `palette-nippon.png` in the file search paths.

You can convert to a different palette with this command. The closest possible color will be chosen for each
color from the source file palette to the specified palette.

## Convert volume to mesh

You can export your volume model into a obj or ply.

`./vengi-voxconvert infile.vox outfile.obj`

Cvars to control the meshing:

* `voxformat_ambientocclusion`: Don't export extra quads for ambient occlusion voxels
* `voxformat_mergequads`: Merge similar quads to optimize the mesh
* `voxformat_reusevertices`: Reuse vertices or always create new ones
* `voxformat_scale`: Scale the vertices by the given factor
* `voxformat_quads`: Export to quads
* `voxformat_withcolor`: Export vertex colors
* `voxformat_withtexcoords`: Export texture coordinates

See `./vengi-voxconvert --help` for details.

## Batch convert

To convert a complete directory of e.g. `*.vox` to `*.obj` files, you can use e.g. the bash like this:

```bash
for i in *.vox; do vengi-voxconvert $i ${i%.vox}.obj; done
```

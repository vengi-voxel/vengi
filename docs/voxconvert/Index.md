# General

Convert voxel volume formats between each other.

Support for loading: vox, qbt, qb, vxm, binvox, cub, kvx, kv6, vxl

Support for writing: vox, qbt, qb, binvox, cub, vxl

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

# Usage

`./vengi-voxconvert --merge --scale --input infile --output outfile`

* `--crop`: reduces the volume sizes to their voxel boundaries.
* `--export-models`: export all the models of a scene into single files. It is suggested to name the models properly to get reasonable file names.
* `--export-palette`: will save the included palette as png next to the source file.
* `--filter <filter>`: will filter out models not mentioned in the expression. E.g. `1-2,4` will handle model 1, 2 and 4. It is the same as `1,2,4`. The first model is `0`. See the models note below.
* `--force`: overwrite existing files
* `--image-as-heightmap`: import input images as heightmap (default)
* `--colored-heightmap`: Use the alpha channel of the heightmap as height and the rgb data as surface color.
* `--image-as-volume`: import given input image as volume. Uses a depth map to make a volume out of the image.
* `--image-as-volume-max-depth`: importing image as volume max depth
* `--image-as-volume-both-sides`: importing image as volume and use the depth map for both sides. The depth-map has the postfix `-dm`. For example the image is called `image.png` then the depth-map image must be called `image-dm.png`. Also see the [examples](Examples.md).
* `--image-as-plane`: import input images as planes
* `--input <file>`: allows to specify input files. You can specify more than one file
* `--merge`: will merge a multi model volume (like `vox`, `qb` or `qbt`) into a single volume of the target file
* `--mirror <x|y|z>`: allows you to mirror the volumes at x, y and z axis
* `--output <file>`: allows you to specify the output filename
* `--resize <x:y:z>`: resize the volume by the given x (right), y (up) and z (back) values
* `--rotate <x|y|z>`: allows you to rotate the volumes by 90 degree at x, y and z axis. Specify e.g. `x:180` to rotate around x by 180 degree.
* `--scale`: perform lod conversion of the input volume (50% scale per call)
* `--script "<script> <args>"`: execute the given script - see [scripting support](../LUAScript.md) for more details
* `--split <x:y:z>`: slices the volumes into pieces of the given size
* `--surface-only`: Remove any non surface voxel.
* `--translate <x:y:z>`: translates the volumes by x (right), y (up), z (back)
* `--wildcard <wildcard>`: e.g. `*.vox`. Allow to specify a wildcard in situations where the `--input` value is a directory

Just type `vengi-voxconvert` to get a full list of commands and options.

Using a different target palette is also possible by setting the `palette` [cvar](../Configuration.md).

`./vengi-voxconvert -set palette /path/to/palette.png --input infile outfile`

The palette file has to be in the dimensions 1x256. It is also possible to just provide the basename of the palette.
This is e.g. `nippon`. The tool will then try to look up the file `palette-nippon.png` in the file search paths.

You can convert to a different palette with this command. The closest possible color will be chosen for each
color from the source file palette to the specified palette.

If something isn't working as you expected it to work, it might be an option to activate the debug logging. But the output can be a bit overwhelming. See the [configuration](../Configuration.md) logging section about more details.

## The order of execution is:

* filter
* export models
* merge
* scale
* mirror
* rotate
* translate
* script
* pivot
* crop
* split

## Models

Some formats also have multiple model support. Our models are maybe not the models you know from your favorite editor. Each model can currently only have one object or volume in it. To get the proper model ids (starting from 0) for your voxel file, you should load it once in [voxedit](../voxedit/Index.md) and check the model panel or use `--dump` to get a list.

Especially magicavoxel supports more objects in one model. This might be confusing to get the right numbers for `voxconvert`. See [this issue](https://github.com/mgerhardy/vengi/issues/68) for a few more details.


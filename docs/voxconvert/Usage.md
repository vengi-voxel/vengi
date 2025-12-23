# Usage

Print a detailed description of the program parameters for your particular version.

```sh
vengi-voxconvert --help
```

> You can use the bash completion or zsh completion
>
> Put this in your `.bashrc` or `.zshrc`
>
> `source <(vengi-voxconvert --completion bash)` (or replace `bash` by `zsh`)

* `--crop`: reduces the volume sizes to their voxel boundaries.
* `--export-models`: export all the models of a scene into single files. It is suggested to name the models properly to get reasonable file names.
* `--export-palette`: will save the palette file for the given input file.
* `--filter <filter>`: will filter out models not mentioned in the expression. E.g. `1-2,4` will handle model 1, 2 and 4. It is the same as `1,2,4`. The first model is `0`. See the models note below.
* `--filter-property <property:value>`: Model filter by property. For example `name:foo`.
* `--force`: overwrite existing files
* `--image`: print the scene voxels to the text console. Useful if you don't have a graphical user interface available but still need to visually compare voxel models.
* `--input <file>`: allows to specify input files. You can specify more than one file
* `--isometric`: Create an isometric thumbnail of the input file when `--image` is used.
* `--json`: Print the scene graph of the input file. Give `full` as argument to also get mesh details.
* `--merge`: will merge a multi model volume (like `vox`, `qb` or `qbt`) into a single volume of the target file
* `--mirror <x|y|z>`: allows you to mirror the volumes at x, y and z axis
* `--output <file>`: allows you to specify the output filename
* `--print-formats`: Print supported formats as json for easier parsing in other tools.
* `--print-scripts`: Print found lua scripts as json for easier parsing in other tools.
* `--resize <x:y:z>`: resize the volume by the given x (right), y (up) and z (back) values
* `--rotate <x|y|z>`: allows you to rotate the volumes by 90 degree at x, y and z axis. Specify e.g. `x:180` to rotate around x by 180 degree.
* `--scale`: perform lod conversion of the input volume (50% scale per call)
* `--script "<script> <args>"`: execute the given script - see [scripting support](../LUAScript.md) for more details
* `--scriptcolor <index>`: Set the palette index that is given to the color script parameters of the main function.
* `--split <x:y:z>`: slices the volumes into pieces of the given size
* `--surface-only`: Remove any non surface voxel. If you are meshing with this, you get also faces on the inner side of your mesh.
* `--translate <x:y:z>`: translates the volumes by x (right), y (up), z (back)
* `--wildcard <wildcard>`: e.g. `*.vox`. Allow to specify a wildcard in situations where the `--input` value is a directory

Just type `vengi-voxconvert` to get a full list of commands and options.

Using a different target palette is also possible by setting the `palette` [cvar](../Configuration.md).

`./vengi-voxconvert -set palette /path/to/palette.png -set voxformat_createpalette false --input infile --output outfile`

The palette file has to be in the dimensions 1x256. It is also possible to just provide the basename of the palette.
This is e.g. `nippon`. The tool will then try to look up the file `palette-nippon.png` in the file search paths.

You can convert to a different palette with this command. The closest possible color will be chosen for each
color from the source file palette to the specified palette.

If something isn't working as you expected it to work, it might be an option to activate the debug logging. But the output can be a bit overwhelming. See the [configuration](../Configuration.md) logging section about more details.

## bash completion

You can also use the bash completion script by adding this to your `.bashrc`

```sh
source <(vengi-voxconvert --completion bash)
```

## The order of execution is:

* filter
* export models
* merge
* scale
* resize
* mirror
* rotate
* translate
* script
* crop
* surface-only
* split

## Models

Some formats also have multiple model support. Our models are maybe not the models you know from your favorite editor. Each model can currently only have one object or volume in it. To get the proper model ids (starting from 0) for your voxel file, you should load it once in [voxedit](../voxedit/Index.md) and check the model panel or use `--json` to get a list.

Especially magicavoxel supports more objects in one model. This might be confusing to get the right numbers for `voxconvert`. See [this issue](https://github.com/vengi-voxel/vengi/issues/68) for a few more details.

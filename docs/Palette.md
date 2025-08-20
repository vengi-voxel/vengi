# Palette

The engine is built around a palette of 256 colors. Custom palettes are supported. The images should have a 1x256 pixel dimension.

You can import palettes from a lot of different image, palette or [voxel volume formats](Formats.md) (if supported).

Each color entry can get [material](Material.md) properties.

The palette can usually be specified by the [cvar](Configuration.md) `palette` and can either be a full path to a png file or an identifier.
If you decide to use the identifier - e.g. `nippon` the `palette` cvar is set to this value and the engine will automatically search
all registered file system paths for a file named `palette-nippon.png`.

There are several built-in palettes available that can also be used as an identifier.

* `built-in:commandandconquer`
* `built-in:magicavoxel`
* `built-in:minecraft`
* `built-in:nippon`
* `built-in:quake1`
* `built-in:starmade`

You can also download and import palettes from Lospec by specifying a palette like this:

* `lospec:paletteid`

This would try to download a palette with the id `paletteid` from [lospec.com](https://lospec.com) in the Gimp (`gpl`) format and automatically imports it.

There are several color or palette related cvars available:

* `voxformat_createpalette`
* `core_colorreduction`
* `palformat_maxsize`
* `palformat_gimprgba`
* `palformat_rgb6bit`

You can find the detailed description and more cvars by using e.g. the [voxconvert](voxconvert/Index.md) `--help` parameter or checking the [Configuration](Configuration.md) documentation.

## Normals

Command & Conquer supports voxel normals. `vengi-voxedit` got a few features to support this. Change the view mode to __Command & Conquer__ to see a normal palette panel and to be able to render the normals for the voxels.

There is also a cvar called `normalpalette` that is used to set the normal palette when importing from meshes.

There are several built-in palettes available that can also be used as an identifier.

* `built-in:redalert2`
* `built-in:tiberiansun`
* `built-in:slab6`

You can also specify a filename to a support palette format to load it.

## core_colorreduction

> Possible values are `Octree`, `Wu`, `NeuQuant`, `KMeans` and `MedianCut`

This [cvar](Configuration.md) controls how the input colors are quantized in the palette colors.

Different input images lead to different results for those options. If you are not pleased with the result on one algorithm, it's often worth the try with another one.

## voxformat_rgbweightedaverage

If you are importing from a mesh format and multiple triangles with different colors would contribute to the same voxel, vengi will try to find the best color by doing an average weighting between all color contributions based on their size. This might lead to new colors - colors there are not part of the input file. If you don't want this, you can disable this feature by setting this [cvar](Configuration.md) to `false`.

## voxformat_rgbflattenfactor

This will reduce the colors for input files with more than 256 colors. It basically does this:

```c
color = color / voxformat_rgbflattenfactor * voxformat_rgbflattenfactor
```

on a per color component base.

This will reduce very similar looking colors and might help to improve the variance in your palette.

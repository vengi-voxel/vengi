# Palette

The engine is built around a palette of 256 colors. Custom palettes are supported. The images should have a 1x256 pixel dimension.

You can import palettes from a lot of different image, palette or [voxel volume formats](Formats.md) (if supported).

The palette can usually be specified by the [cvar](Configuration.md) `palette` and can either be a full path to a png file or an identifier.
If you decide to use the identifier - e.g. `nippon` the `palette` cvar is set to this value and the engine will automatically search
all registered file system paths for a file named `palette-nippon.png`.

There are several built-in palettes available that can also be used as an identifier.

* `built-in:commandandconquer`
* `built-in:magicavoxel`
* `built-in:minecraft`
* `built-in:nippon`
* `built-in:quake1`

You can also download and import palettes from Lospec by specifying a palette like this:

* `lospec:paletteid`

This would try to download a palette with the id `paletteid` from [lospec.com](https://lospec.com) in the Gimp (`gpl`) format and automatically imports it.

There are several color or palette related cvars available:

* `voxformat_createpalette`: Allows you to disable the palette creation and use the palette specified via `palette` cvar
* `core_colorreduction`: Allows you to specify a color reduction value when e.g. importing RGB(A) based voxel or mesh formats

You can find the detailed description and more cvars by using e.g. the [voxconvert](voxconvert/Index.md) `--help` parameter

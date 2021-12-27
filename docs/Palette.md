# Palette

The engine is built around a palette of 256 colors. Custom palettes are supported. The images should have a 1x256 pixel dimension.

You can import palettes from a lot of different image formats - or from [voxel volume formats](Formats.md) (if supported).

The palette can usually be specified by a [cvar](Configuration.md) and can either be a full path to a png file or an identifier.
If you decide to use the identifier - e.g. `nippon` the palette cvar is set to this value and the engine will automatically search
all registered file system paths for a file named `palette-nippon.png`.

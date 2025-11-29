# Examples

[![asciicast](https://asciinema.org/a/723546.svg)](https://asciinema.org/a/723546)

## Convert palette

Convert Adobe Swatch Exchange palette file to png

`./vengi-palconvert --input infile.ase --output outfile.png`

## Create palette from image

This is removing fully transparent color entries and removes duplicates

```sh
vengi-palconvert --input yourimage.png --optimize --output yourpalette.png
```

## Convert from Gimp GPL palette to Magicavoxel png

If your `gpl` palette has more than 256 colors, you either might want to use `--quantize` or `--optimize`.

```sh
vengi-palconvert --input yourgimp.gpl --output magicavoxel.png
```

# General

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/thumbnailer.jpg)

Create thumbnailer images of all supported voxel formats. In combination with a mimetype definition and a `.thumbnailer` definition file
that must be installed in `/usr/share/mime/packages` and `/usr/share/thumbnailer` this will e.g. create small preview images for
`vox`, `qb`, `qbt`, `vxm`, `cub`, `vxl`, ... files.

It works for any file manager that supports `.thumbnailer` entries, including Nautilus, Thunar (when tumbler is installed), Nemo, Caja,
and PCManFM.

## Example

This allows you to create the thumbnails manually.

```bash
for i in $(find $HOME/dev/engine -name "*.vox" -or -name "*.cub" -or -name "*.qbt" -or -name "*.qb" -or -name "*.vxl" -or -name "*.vxm"); do
 fullpath=$(readlink -f $i)
 md5=$(echo -n "file://$fullpath" | md5sum -z | awk ' { print $1.".png" }')
 vengi-thumbnailer -s 128 $i $HOME/.cache/thumbnails/large/$md5
done
```

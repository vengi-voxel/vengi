# Example

This allows you to create the thumbnails manually.

```bash
for i in $(find $HOME/dev/engine -name "*.vox" -or -name "*.cub" -or -name "*.qbt" -or -name "*.qb" -or -name "*.vxl" -or -name "*.vxm"); do
 fullpath=$(readlink -f $i)
 md5=$(echo -n "file://$fullpath" | md5sum -z | awk ' { print $1.".png" }')
 vengi-thumbnailer -s 128 $i $HOME/.cache/thumbnails/large/$md5
done
```

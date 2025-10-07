# Example

## Manual thumbnails creation

### Unix

```sh
for i in $(find $HOME/dev/vengi -name "*.vox" -or -name "*.cub" -or -name "*.qbt" -or -name "*.qb" -or -name "*.vxl" -or -name "*.vxm"); do
 fullpath=$(readlink -f $i)
 md5=$(echo -n "file://$fullpath" | md5sum -z | awk ' { print $1.".png" }')
 vengi-thumbnailer -s 128 --input $i --output $HOME/.cache/thumbnails/large/$md5
done
```

### Windows

```ps
$array = "1-2,5", "1-2,7"
foreach ($i in $array){
  ./vengi-thumbnailer.exe -s 128 --input $i --output $i.png
}
```

## Turntables

The thumbnailer is able to generate scene turntables with 16 images.

```sh
./vengi-thumbnailer -s 128 --turntable 16 --input somevoxel.vox --output somevoxel.png
```

## Render top view

```sh
./vengi-thumbnailer -s 128 --camera-mode top --input somevoxel.vox --output somevoxel.png
```

## Render isometric image


```sh
./vengi-thumbnailer -s 128 --input somevoxel.vox --output somevoxel.png --image --isometric
```

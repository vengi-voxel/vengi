# Formats

## Voxel formats

> The `vengi` format is the best supported format. Saving into any other format might lose several details from your scene. This depends on the capabilities of the format and the completeness of the implementation for supporting that particular format.

| Name                       | Extension | Loading | Saving | Thumbnails | Palette | Animations | Spec                                                                     |
| :------------------------- | --------- | ------- | ------ | ---------- | ------- | ---------- | ------------------------------------------------------------------------ |
| Ace Of Spades              | vxl       | X       | X      | X          | X       |            | [spec](https://silverspaceship.com/aosmap/aos_file_format.html)          |
| BinVox                     | binvox    | X       | X      | X          |         |            | [spec](https://www.patrickmin.com/binvox/binvox.html)                    |
| Build engine               | kvx       | X       |        | X          | X       |            | [spec](https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt)     |
| Chronovox-Studio           | csm       | X       |        | X          |         |            |                                                                          |
| Command and Conquer        | vxl/hva   | X       | X      | X          | X       | X          | [spec](http://xhp.xwis.net/documents/VXL_Format.txt)                     |
| CubeWorld                  | cub       | X       | X      | X          | X       |            |                                                                          |
| Goxel                      | gox       | X       | X      | X          | X       |            |                                                                          |
| MagicaVoxel                | vox       | X       | X      | X          | X       |            | [spec](https://github.com/ephtracy/voxel-model)                          |
| Minecraft Level            | dat       | X       |        | X          | X       |            |                                                                          |
| Minecraft Region           | mcr       | X       | X      | X          | X       |            | [spec](https://minecraft.gamepedia.com/Region_file_format)               |
| Minecraft Schematics       | schematic | X       |        | X          | X       |            | [spec](https://minecraft.fandom.com/wiki/Schematic_file_format)          |
| Minecraft Schematics       | schem     | X       |        | X          | X       |            | [spec](https://minecraft.fandom.com/wiki/Schematic_file_format)          |
| Minecraft Schematics       | nbt       | X       |        | X          | X       |            | [spec](https://minecraft.fandom.com/wiki/Schematic_file_format)          |
| Minetest                   | mts       | X       |        | X          | X       |            | [spec](https://dev.minetest.net/Minetest_Schematic_File_Format)          |
| Nick's Voxel Model         | nvm       | X       |        | X          |         |            |                                                                          |
| Qubicle Binary Tree        | qbt       | X       | X      | X          | X       |            | [spec](https://getqubicle.com/qubicle/documentation/docs/file/qbt/)      |
| Qubicle Binary             | qb        | X       | X      | X          | X       |            | [spec](https://getqubicle.com/qubicle/documentation/docs/file/qb/)       |
| Qubicle Exchange           | qef       | X       | X      | X          |         |            | [spec](https://getqubicle.com/qubicle/documentation/docs/file/qef/)      |
| Qubicle Project            | qbcl      | X       | X      | X          | X       |            | [spec](https://gist.github.com/tostc/7f049207a2e5a7ccb714499702b5e2fd)   |
| Sandbox VoxEdit Collection | vxc       | X       |        |            |         |            |                                                                          |
| Sandbox VoxEdit Model      | vxm       | X       | X      | X          | X       |            |                                                                          |
| Sandbox VoxEdit Hierarchy  | vxr       | X       | X      | X          |         | X          |                                                                          |
| Sandbox VoxEdit Tileset    | vxt       | X       |        |            |         |            |                                                                          |
| SLAB6                      | kv6       | X       | X      | X          | X       |            | [spec](https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt)     |
| Sproxel                    | csv       | X       | X      | X          | X       |            | [spec](https://github.com/emilk/sproxel/blob/master/ImportExport.cpp)    |
| StarMade                   | sment     | X       |        | X          | X       |            | [spec](https://starmadepedia.net/wiki/Blueprint_File_Formats)            |
| Vengi                      | vengi     | X       | X      | X          | X       | X          |                                                                          |
| VoxelMax                   | vmax      |         |        | X          | X       |            |                                                                          |

## Mesh formats

| Name                       | Extension | Loading | Saving    |
| :------------------------- | --------- | ------- | --------- |
| Filmbox                    | fbx       | X       | X (ascii) |
| GL Transmission Format     | gltf, glb | X       | X         |
| Quake 1/UFO:Alien Invasion | bsp       | X       |           |
| Quake 2 Model              | md2       | X       |           |
| Standard Triangle Language | stl       | X       | X         |
| Wavefront Object           | obj       | X       | X         |

## Palettes

| Name                        | Extension | Loading | Saving |
| :-------------------------- | --------- | ------- | ------ |
| Portable Network Graphics   | png       | X       | X      |
| Gimp Palette                | gpl       | X       | X      |
| Qubicle Palette             | qsm       | X       |        |
| RGB Palette                 | pal       | X       | X      |
| CSV Palette                 | csv       | X       | X      |

## Images/textures

| Name                        | Extension |
| :-------------------------- | --------- |
| Portable Network Graphics   | png       |
| JPEG                        | jpeg, jpg |
| Targa image file            | tga       |
| DDS                         | dds       |
| PKM                         | pkm       |
| PVR                         | pvr       |
| Bitmap                      | bmp       |
| Photoshop                   | psd       |
| Graphics Interchange Format | gif       |
| Radiance rgbE               | hdr       |
| Softimage PIC               | pic       |
| Portable Anymap             | pnm       |

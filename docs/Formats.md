# Formats

## Voxel formats

> The [vengi](FormatSpec.md) format is the best supported format. Saving into any other format might lose several details from your scene. This depends on the capabilities of the format and the completeness of the implementation for supporting that particular format.

| Name                       | Extension   | Loading | Saving | Thumbnails | Palette | Animations |
| :------------------------- | ----------- | ------- | ------ | ---------- | ------- | ---------- |
| AceOfSpades                | kv6         | X       | X      |            | X       |            |
| AceOfSpades                | vxl         | X       | X      |            | X       |            |
| AnimaToon                  | scn         | X       |        |            |         | X          |
| aseprite                   | aseprite    | X       |        |            | X       |            |
| BenVoxel                   | ben.json    | X       | X      |            | X       |            |
| BinVox                     | binvox      | X       | X      |            |         |            |
| Build engine               | kvx         | X       | X      |            | X       |            |
| Chronovox                  | csm         | X       |        |            |         |            |
| CubeWorld                  | cub         | X       | X      |            | X       |            |
| Cubzh                      | 3zh         | X       | X      | X          | X       |            |
| Cubzh World                | b64         | X       |        |            |         |            |
| Goxel                      | gox         | X       | X      | X          | X       |            |
| Goxel txt                  | txt         | X       | X      |            |         |            |
| KenShape                   | kenshape    | X       |        |            | X       |            |
| Luanti (Minetest)          | mts         | X       | X      |            | X       |            |
| MagicaVoxel                | vox         | X       | X      |            | X       |            |
| Magicavoxel XRAW           | xraw        | X       | X      |            | X       |            |
| Minecraft level dat        | dat         | X       |        |            |         |            |
| Minecraft mcworld          | mcworld     | X       |        |            |         |            |
| Minecraft region           | mca         | X       |        |            | X       |            |
| Minecraft schematic        | schematic   | X       | X      |            | X       |            |
| Minecraft skin             | mcskin      | X       | X      |            | X       |            |
| Nicks Voxel Model          | nvm         | X       |        |            |         |            |
| Particubes                 | pcubes      | X       | X      | X          | X       |            |
| Portable Network Graphics  | png         | X       | X      |            |         |            |
| Qubicle Binary             | qb          | X       | X      |            | X       |            |
| Qubicle Binary Tree        | qbt         | X       | X      |            | X       |            |
| Qubicle Exchange           | qef         | X       | X      |            |         |            |
| Qubicle Project            | qbcl        | X       | X      | X          | X       |            |
| Rooms.xyz Thing            | thing       | X       |        |            | X       |            |
| Sandbox VoxEdit Block      | vxb         | X       | X      |            | X       |            |
| Sandbox VoxEdit Collection | vxc         | X       |        | X          |         |            |
| Sandbox VoxEdit Hierarchy  | vxr         | X       | X      |            |         | X          |
| Sandbox VoxEdit Model      | vxm         | X       | X      |            | X       |            |
| Sandbox VoxEdit Tilemap    | vxt         | X       |        |            |         |            |
| SLAB6 vox                  | vox         | X       | X      |            | X       |            |
| SpriteStack                | zip         | X       | X      |            | X       |            |
| Sproxel csv                | csv         | X       | X      |            | X       |            |
| StarMade Blueprint         | sment       | X       |        |            | X       |            |
| StarMade Template          | smtpl       | X       | X      |            | X       |            |
| Tiberian Sun               | vxl         | X       | X      |            | X       | X          |
| Veloren terrain            | dat         | X       | X      |            | X       |            |
| Vengi                      | vengi       | X       | X      |            | X       | X          |
| Voxel3D                    | v3a         | X       | X      |            |         |            |
| VoxelBuilder               | vbx         | X       |        |            |         |            |
| VoxelMax                   | vmax.zip    | X       |        | X          | X       |            |

## Mesh formats

| Name                       | Extension | Loading | Saving    | Animations |
| :------------------------- | --------- | ------- | --------- | ---------- |
| Autodesk 3D Studio         | 3ds       | X       |           |            |
| Blockbench                 | bbmodel   | X       |           |            |
| FBX                        | fbx       | X       | X         |            |
| GL Transmission Format     | gltf      | X       | X         | X          |
| Godot Scene                | escn      |         | X         |            |
| Polygon File Format        | ply       | X       | X         |            |
| Quake 1                    | bsp       | X       |           |            |
| Quake 1 Model              | mdl       | X       |           |            |
| Quake 2 Model              | md2       | X       |           |            |
| Quake Map                  | map       | X       |           |            |
| Standard Triangle Language | stl       | X       | X         |            |
| UFO:Alien Invasion         | bsp       | X       |           |            |
| Wavefront Object           | obj       | X       | X         |            |

Point cloud support for `ply` and `gtlf` is implemented, too.

## Palettes

| Name                            | Extension | Loading | Saving |
| :------------------------------ | --------- | ------- | ------ |
| Adobe Color Bock                | acb       | X       | X      |
| Adobe Swatch Exchange           | ase       | X       | X      |
| Avoyd Material                  | avmt      | X       | X      |
| CSV Palette                     | csv       | X       | X      |
| Gimp Palette                    | gpl       | X       | X      |
| JASC Palette                    | pal       | X       | X      |
| Paint.NET Palette               | txt       | X       | X      |
| Photoshop Palette               | aco       | X       | X      |
| Pixelorama                      | json      | X       | X      |
| Portable Network Graphics       | png       | X       | X      |
| Qubicle Palette                 | qsm       | X       |        |
| RGB Palette                     | pal       | X       | X      |
| Tiberian Sun Palette            | vpl       | X       |        |

> The `gpl` format also supports the [Aseprite extension](https://github.com/aseprite/aseprite/blob/main/docs/gpl-palette-extension.md) for alpha values

## Images/textures

| Name                        | Extension |
| :-------------------------- | --------- |
| Bitmap                      | bmp       |
| DDS                         | dds       |
| Graphics Interchange Format | gif       |
| JPEG                        | jpeg      |
| Photoshop                   | psd       |
| PKM                         | pkm       |
| Portable Anymap             | pnm       |
| Portable Network Graphics   | png       |
| PVR                         | pvr       |
| Radiance rgbE               | hdr       |
| Softimage PIC               | pic       |
| Targa image file            | tga       |

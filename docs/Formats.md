# Formats

| Name                       | Extension | Loading | Saving | Thumbnails | Palette | Animations | Spec                                                                     |
| :------------------------- | --------- | ------- | ------ | ---------- | ------- | ---------- | ------------------------------------------------------------------------ |
| Ace Of Spades              | vxl       | X       | X      | X          | X       |            | [spec](https://silverspaceship.com/aosmap/aos_file_format.html)          |
| BinVox                     | binvox    | X       | X      | X          |         |            | [spec](https://www.patrickmin.com/binvox/binvox.html)                    |
| Build engine               | kvx       | X       |        | X          | X       |            | [spec](https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt)     |
| Chronovox-Studio           | csm       | X       |        | X          |         |            |                                                                          |
| Command and Conquer        | vxl/hva   | X       | X      | X          | X       | X          | [spec](http://xhp.xwis.net/documents/VXL_Format.txt)                     |
| CubeWorld                  | cub       | X       | X      | X          | X       |            |                                                                          |
| GL Transmission Format     | gltf      | X       | X      |            |         |            | [spec](https://github.com/KhronosGroup/glTF/tree/main/specification/2.0) |
| Goxel                      | gox       | X       | X      | X          | X       |            |                                                                          |
| MagicaVoxel                | vox       | X       | X      | X          | X       |            | [spec](https://github.com/ephtracy/voxel-model)                          |
| Minecraft Level            | dat       | X       |        | X          | X       |            |                                                                          |
| Minecraft Region           | mcr       | X       | X      | X          | X       |            | [spec](https://minecraft.gamepedia.com/Region_file_format)               |
| Minecraft Schematics       | schematic | X       |        | X          | X       |            | [spec](https://minecraft.fandom.com/wiki/Schematic_file_format)          |
| Minecraft Schematics       | schem     | X       |        | X          | X       |            | [spec](https://minecraft.fandom.com/wiki/Schematic_file_format)          |
| Minecraft Schematics       | nbt       | X       |        | X          | X       |            | [spec](https://minecraft.fandom.com/wiki/Schematic_file_format)          |
| Nick's Voxel Model         | nvm       | X       |        | X          |         |            |                                                                          |
| Quake 1/UFO:Alien Invasion | bsp       | X       |        |            |         |            |                                                                          |
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
| Standard Triangle Language | stl       | X       | X      |            |         |            |                                                                          |
| Wavefront Object           | obj       | X       | X      |            |         |            |                                                                          |


## Meshes

Exporting to ply, gltf, stl and obj is also supported. A few [cvars](Configuration.md) exists to tweak the output of the meshing:

* `voxformat_ambientocclusion`: Don't export extra quads for ambient occlusion voxels
* `voxformat_mergequads`: Merge similar quads to optimize the mesh
* `voxformat_reusevertices`: Reuse vertices or always create new ones
* `voxformat_scale`: Scale the vertices on all axis by the given factor
* `voxformat_scale_x`: Scale the vertices on X axis by the given factor
* `voxformat_scale_y`: Scale the vertices on Y axis by the given factor
* `voxformat_scale_z`: Scale the vertices on Z axis by the given factor
* `voxformat_quads`: Export to quads
* `voxformat_withcolor`: Export vertex colors
* `voxformat_withtexcoords`: Export texture coordinates
* `voxformat_transform_mesh`: Apply the keyframe transform to the mesh
* `voxformat_marchingcubes`: Use the marching cubes algorithm to produce the mesh
* `voxformat_createpalette`: Setting this to false will use use the palette configured by `palette` cvar and use those colors as a target. This is mostly useful for meshes with either texture or vertex colors or when importing rgba colors. This is not used for palette based formats.

Basic voxelization is supported for ply, gltf, stl, bsp and obj files, too. The following [cvars](Configuration.md) can be modified here:

* `voxformat_fillhollow`: Fill the inner parts of completely close objects
* `voxformat_scale`: Scale the vertices on all axis by the given factor
* `voxformat_scale_x`: Scale the vertices on X axis by the given factor
* `voxformat_scale_y`: Scale the vertices on Y axis by the given factor
* `voxformat_scale_z`: Scale the vertices on Z axis by the given factor
* `voxformat_voxel_mesh`: Optimize import precision assuming that the mesh is composed of uniform voxels

## Format configuration

Next to the mesh related configuration cvars there are a few format specific ones, too:

* `voxformat_vxlnormaltype`: Normal type for VXL format - 2 (TS) or 4 (RedAlert2)
* `voxformat_qbtpalettemode`: Use palette mode in qubicle qbt export
* `voxformat_qbtmergecompounds`: Merge compounds in qbt export
* `voxformat_voxcreatelayers`: Magicavoxel vox layers
* `voxformat_voxcreategroups`: Magicavoxel vox groups
* `voxformat_merge`: Merge all models into one object
* `voxformat_rgbflattenfactor`: To flatten the RGB colors when importing volumes (0-255)
* `voxformat_qbsavelefthanded`: Save qubicle format as left handed

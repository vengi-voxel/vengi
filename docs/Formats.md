# Formats

| Name                       | Extension | Loading | Saving | Thumbnails | Palette | Binary | Spec                                                             |
| :------------------------- | --------- | ------- | ------ | ---------- | ------- | ------ | ---------------------------------------------------------------- |
| Ace Of Spades              | vxl       | X       |        | X          |         | X      |                                                                  |
| BinVox                     | binvox    | X       | X      | X          |         | X      | https://www.patrickmin.com/binvox/binvox.html                    |
| Build engine               | kvx       | X       |        | X          | X       | X      | https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt     |
| Chronovox-Studio           | csm       | X       |        | X          |         | X      |                                                                  |
| Command and Conquer        | vxl       | X       | X      | X          | X       | X      |                                                                  |
| CubeWorld                  | cub       | X       | X      | X          |         | X      |                                                                  |
| Goxel                      | gox       | X       | X      | X          |         | X      |                                                                  |
| MagicaVoxel                | vox       | X       | X      | X          | X       | X      | ttps://github.com/ephtracy/voxel-model                           |
| Minecraft Region           | mcr       | X       |        | X          | X       | X      | https://minecraft.gamepedia.com/Region_file_format               |
| Minecraft Schematics       | schematic | X       |        | X          | X       | X      | https://minecraft.fandom.com/wiki/Schematic_file_format          |
| Nick's Voxel Model         | nvm       | X       |        | X          |         | X      |                                                                  |
| Qubicle Exchange           | qef       | X       | X      | X          |         |        | https://getqubicle.com/qubicle/documentation/docs/file/qef/      |
| Qubicle Binary Tree        | qbt       | X       | X      | X          |         | X      | https://getqubicle.com/qubicle/documentation/docs/file/qbt/      |
| Qubicle Binary             | qb        | X       | X      | X          |         | X      | https://getqubicle.com/qubicle/documentation/docs/file/qb/       |
| Qubicle Project            | qbcl      | X       | X      | X          |         | X      |                                                                  |
| Sandbox VoxEdit Collection | vxc       | X       |        |            |         | X      |                                                                  |
| Sandbox VoxEdit Model      | vxm       | X       | X      | X          | X       | X      |                                                                  |
| Sandbox VoxEdit            | vxr       | X       | X      | X          |         | X      |                                                                  |
| Sandbox VoxEdit Tile       | vxt       | X       |        |            |         | X      |                                                                  |
| SLAB6                      | kv6       | X       |        | X          | X       | X      | https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt     |
| Sproxel                    | csv       | X       | X      | X          |         |        |                                                                  |
| Wavefront Object           | obj       | X       | X      |            |         |        |                                                                  |
| Standard Triangle Language | stl       | X       | X      |            |         |        |                                                                  |
| GL Transmission Format     | gltf      | X       | X      |            |         | X      | https://github.com/KhronosGroup/glTF/tree/main/specification/2.0 |


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

Basic voxelization is supported for ply, gltf, stl and obj files, too. The following [cvars](Configuration.md) can be modified here:

* `voxformat_scale`: Scale the vertices on all axis by the given factor
* `voxformat_scale_x`: Scale the vertices on X axis by the given factor
* `voxformat_scale_y`: Scale the vertices on Y axis by the given factor
* `voxformat_scale_z`: Scale the vertices on Z axis by the given factor

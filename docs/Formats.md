# Formats

| Name                       | Extension | Loading | Saving | Thumbnails | Palette |
| :------------------------- | --------- | ------- | ------ | ---------- | ------- |
| Ace Of Spades              | vxl       | X       |        | X          |         |
| BinVox                     | binvox    | X       | X      | X          |         |
| Build engine               | kvx       | X       |        | X          | X       |
| Chronovox-Studio           | csm       | X       |        | X          |         |
| Command and Conquer        | vxl       | X       | X      | X          | X       |
| CubeWorld                  | cub       | X       | X      | X          |         |
| Goxel                      | gox       | X       | X      | X          |         |
| MagicaVoxel                | vox       | X       | X      | X          | X       |
| Minecraft Region           | mcr       | X       |        | X          | X       |
| Nick's Voxel Model         | nvm       | X       |        | X          |         |
| Qubicle Exchange           | qef       | X       | X      | X          |         |
| Qubicle Binary Tree        | qbt       | X       | X      | X          |         |
| Qubicle Binary             | qb        | X       | X      | X          |         |
| Qubicle Project            | qbcl      | X       | X      | X          |         |
| Sandbox VoxEdit Collection | vxc       | X       |        |            |         |
| Sandbox VoxEdit Model      | vxm       | X       | X      | X          | X       |
| Sandbox VoxEdit            | vxr       | X       | X      | X          |         |
| Sandbox VoxEdit Tile       | vxt       | X       |        |            |         |
| SLAB6                      | kv6       | X       |        | X          | X       |
| Sproxel                    | csv       | X       | X      | X          |         |
| Wavefront Object           | obj       | X       | X      |            |         |
| Standard Triangle Language | stl       | X       | X      |            |         |
| GL Transmission Format     | gltf      |         | X      |            |         |


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

Basic voxelization is supported for obj and stl files, too.

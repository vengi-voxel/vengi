# Formats

| Name                  | Extension | Loading | Saving | Thumbnails | Palette |
| :-------------------- | --------- | ------- | ------ | ---------- | ------- |
| Ace Of Spades         | vxl       | X       |        | X          |         |
| BinVox                | binvox    | X       | X      | X          |         |
| Build engine          | kvx       | X       |        | X          | X       |
| Chronovox-Studio      | csm       | X       |        | X          |         |
| Command and Conquer   | vxl       | X       | X      | X          | X       |
| CubeWorld             | cub       | X       | X      | X          |         |
| Goxel                 | gox       | X       | X      | X          |         |
| MagicaVoxel           | vox       | X       | X      | X          | X       |
| Minecraft Region      | mcr       | X       |        | X          | X       |
| Nick's Voxel Model    | nvm       | X       |        | X          |         |
| Qubicle Exchange      | qef       | X       | X      | X          |         |
| Qubicle Binary Tree   | qbt       | X       | X      | X          |         |
| Qubicle Binary        | qb        | X       | X      | X          |         |
| Qubicle Project       | qbcl      | X       |        | X          |         |
| Sandbox VoxEdit       | vxm       | X       | X      | X          | X       |
| Sandbox VoxEdit       | vxr       | X       | X      | X          |         |
| SLAB6                 | kv6       | X       |        | X          | X       |
| Sproxel               | csv       | X       | X      | X          |         |
| Wavefront Object      | obj       | X       | X      |            |         |


## Meshes

Exporting to ply and obj is also supported. A few [cvars](Configuration.md) exists to tweak the output of the meshing:

* `voxformat_ambientocclusion`: Don't export extra quads for ambient occlusion voxels
* `voxformat_mergequads`: Merge similar quads to optimize the mesh
* `voxformat_reusevertices`: Reuse vertices or always create new ones
* `voxformat_scale`: Scale the vertices by the given factor
* `voxformat_quads`: Export to quads
* `voxformat_withcolor`: Export vertex colors
* `voxformat_withtexcoords`: Export texture coordinates

Basic voxelization is supported for obj files, too.

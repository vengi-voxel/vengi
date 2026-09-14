# Blender addon

The addon at `src/tools/voxconvert/blender/io_vengi_voxconvert` wraps the external `vengi-voxconvert` binary. It does **not** keep voxels inside Blender. Import converts a voxel file to a mesh preview:

`voxel file -> vengi-voxconvert -> GLB -> Blender's bundled glTF 2.0 importer`

Export is the reverse:

`Blender mesh -> bundled glTF 2.0 exporter -> GLB -> vengi-voxconvert -> voxel file`

The glTF step uses Blender's **bundled Khronos glTF 2.0 add-on** (`Import-Export: glTF 2.0`). Operators are `bpy.ops.import_scene.gltf` and `bpy.ops.export_scene.gltf` from module `io_scene_gltf2` (Blender 4.2+: also `bl_ext.blender_org.io_scene_gltf2`). Enable that add-on if import/export reports it is missing.

## Install the zip

**Blender 3.6 - 4.1:** Edit > Preferences > Add-ons > Install... and choose `io_vengi_voxconvert.zip`.

**Blender 4.2+:** Edit > Preferences > Get Extensions > Install from Disk... and choose the same zip (`blender_manifest.toml` makes it an extension).

Then set **vengi-voxconvert Path** in the addon preferences if it was not found automatically.

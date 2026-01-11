# Configuration

## Variables

The engine can get configured by so called cvars (configuration variables). These variables can be modified from within
the game via key bindings, built-in console, the ui or scripts.

To get a list of supported cvars (they might differ from application to application), type the command `cvarlist` to the
built-in console - or execute the application with `--help` (Example: `vengi-voxedit.exe --help`).

The variables can get their initial value from various sources. The highest order is the command line. If you specify it on
the command line, every other method will not be used. If the engine finds the cvar name in your environment variables, this
one will take precedence over the one that is found in the configuration file. Next is the configuration file - this one will
take precedence over the default settings that are specified in the code.

The environment variable can be either lower case or upper case. For example it will work if you have `CL_GAMMA` or `cl_gamma`
exported. The lower case variant has the higher priority.

### Commandline

```bash
./vengi-voxvonvert -set voxformat_scale 2.0 [...]
```

### Environment

```bash
export VOXFORMAT_SCALE=2.0
./vengi-voxconvert [...]
```

## Configuration file

* Linux: `~/.local/share/vengi/voxconvert/voxconvert.vars`
* Windows: `C:/Users/bob/AppData/Roaming/vengi/voxconvert/voxconvert.vars`
* Mac: `/Users/bob/Library/Application Support/vengi/voxconvert/voxconvert.vars`

These folders not only contain the configuration file, but also the logs and other files that are created by the application. `log.txt` and `crash.txt` are the most important files that you can attach to a bug report.

## Search paths

You can get a list of search paths by calling the particular application with the `--help` parameter on the command line. This will print a list of search paths. You can even add your own paths by setting the cvar `core_path`.

> For packagers it might be interesting to set the `PKGDATADIR` cmake variable.

## Commands

To get a list of supported commands (they might differ from application to application), type the command `cmdlist` to the built-in console.

You can also get a list when doing `./vengi-app --help` on the command line.

## Key bindings

You can also modify or add key bindings to commands. Type `bindlist` to the console to get a list of the current active bindings
(and also here: they might differ from application to application). The command `bind` can be used to configure keybindings on-the-fly. These bindings are saved to a file on shutdown.

VoxEdit has an ui panel to show the configured keybindings - see [controls](voxedit/Controls.md) for more details.

## Logging

You can either log via syslog (on unix) or to stdout (this might of course differ from platform to platform).

The log level is configured by the `core_loglevel` variable. The lower the value, the more you see. `1` is the highest log level
(trace), where 5 is the lowest log level (error).

* `trace`: 1
* `debug`: 2
* `info`: 3
* `warn`: 4
* `error`: 5

## External tools

External tools can e.g. control the editor by first starting it with `app_pipe` being set to `true`. This will open a pipe named `vengi-<app>-input`.

You can e.g. send the command `xs noise` to the pipe of the running editor instance to execute the `noise.lua` [script](LUAScript.md).

> `echo "xs noise" > ~/.local/share/vengi/voxedit/.vengi-<app>-input` (Linux/Mac)
> `echo "xs noise" > \\.\pipe\vengi-<app>-input` (Windows)

## General

To get a rough usage overview, you can start any application with `--help`. It will print out the commands and configuration variables
with a description and hints how to modify/use them.

## Video settings

| Name                          | Description                                                                              |
| ----------------------------- | ---------------------------------------------------------------------------------------- |
| `cl_vsync`                    | enable or disable v-sync                                                                 |
| `cl_gamma`                    | tweak the gamma value that is applied last on rendering                                  |
| `cl_display`                  | the display index if you are using multiple monitors `[0-numDisplays)`                   |

## Voxel settings

A few cvars exists to tweak the export or import of several formats.

Some of these settings are only for voxel format, others are only for the mesh formats like ply, gltf, stl, fbx and obj.

| Name                          | Description                                                                              | Example      |
| ----------------------------- | ---------------------------------------------------------------------------------------- | ------------ |
| `core_colorreduction`         | This can be used to tweak the color reduction by switching to a different algorithm. Possible values are `Octree`, `Wu`, `NeuQuant`, `KMeans` and `MedianCut`. This is useful for mesh based formats or RGBA based formats like e.g. AceOfSpades vxl. | Octree       |
| `palformat_gimprgba`          | Use RGBA format for GIMP palettes (instead of RGB / Aseprite extension)                  | true/false   |
| `palformat_maxsize`           | The maximum size of an image in x and y direction to quantize to a palette               | 512          |
| `palformat_rgb6bit`           | Use 6 bit color values for the palette (0-63) - used e.g. in C&C pal files               | true/false   |
| `voxel_meshmode`              | Set to 1 to use the marching cubes algorithm to produce the mesh                         | 0/1/2        |
| `voxformat_ambientocclusion`  | Don't export extra quads for ambient occlusion voxels                                    | true/false   |
| `voxformat_binvoxversion`     | Save in version 1, 2 or the unofficial version 3                                         | 1/2/3        |
| `voxformat_colorasfloat`      | Export the vertex colors as float or - if set to false - as byte values (GLTF/Unreal)    | true/false   |
| `voxformat_createpalette`     | Setting this to false will use use the palette configured by `palette` cvar and use those colors as a target. This is mostly useful for meshes with either texture or vertex colors or when importing rgba colors. This is not used for palette based formats - but also for RGBA based formats. | true/false   |
| `voxformat_emptypaletteindex` | By default this is `-1` which means that no color is skipped. Pick 0-255 to remove that palette index from the final saved file. **NOTE**: this only works for formats that don't force the empty voxel to be `0` or `255` (or any other index) already |
| `voxformat_fillhollow`        | Fill the inner parts of completely close objects, when voxelizing a mesh format. To fill the inner parts for non mesh formats, you can use the fillhollow.lua script. | true/false   |
| `voxformat_gltf_khr_materials_pbrspecularglossiness` | Apply KHR_materials_pbrSpecularGlossiness extension on saving gltf files           | true/false   |
| `voxformat_gltf_khr_materials_specular`              | Apply KHR_materials_specular extension on saving gltf files                        | true/false   |
| `voxformat_imageheightmapminheight`                  | The minimum height of the heightmap when importing an image as heightmap           | 0            |
| `voxformat_imageimporttype`                          | 0 = plane, 1 = heightmap, 2 = volume                                               | 0            |
| `voxformat_imagesavetype`                            | 0 = plane, 1 = heightmap, 3 = thumbnail                                            | 0            |
| `voxformat_imagesliceoffsetaxis`                     | The axis to offset the slices when importing images as volumes or heightmaps       | x/y/z        |
| `voxformat_imagesliceoffset`                         | The offset of the slices when importing images as volumes or heightmaps            | 0            |
| `voxformat_imagevolumemaxdepth`                      | The maximum depth of the volume when importing an image as volume                  | 1            |
| `voxformat_imagevolumebothsides`                     | Import the image as volume for both sides                                          | true/false   |
| `voxformat_mergequads`        | Merge similar quads to optimize the mesh                                                 | true/false   |
| `voxformat_merge`             | Merge all models into one object                                                         | true/false   |
| `voxformat_mesh_simplify`     | Simplify the mesh before voxelizing it                                                   | true/false   |
| `voxformat_optimize`          | Apply mesh optimizations when saving mesh based formats                                  | true/false   |
| `voxformat_pointcloudsize`    | Specify the side length for the voxels when loading a point cloud                        | 1            |
| `voxformat_qbtpalettemode`    | Use palette mode in qubicle qbt export                                                   | true/false   |
| `voxformat_qbtmergecompounds` | Merge compounds in qbt export                                                            | true/false   |
| `voxformat_qbsavelefthanded`  | Save qubicle format as left handed                                                       | true/false   |
| `voxformat_qbsavecompressed`  | Save qubicle with RLE compression                                                        | true/false   |
| `voxformat_quads`             | Export to quads                                                                          | true/false   |
| `voxformat_reusevertices`     | Reuse vertices or always create new ones                                                 | true/false   |
| `voxformat_rgbflattenfactor`  | To flatten the RGB colors when importing volumes (0-255) from RGBA or mesh based formats | 1            |
| `voxformat_rgbweightedaverage`| If multiple triangles contribute to the same voxel the color values are averaged based on their area contribution | true/false   |
| `voxformat_savevisibleonly`   | Save only visible nodes                                                                  | true/false |
| `voxformat_scale`             | Scale the vertices for voxelization on all axis by the given factor                      | 1.0          |
| `voxformat_scale_x`           | Scale the vertices for voxelization on X axis by the given factor                        | 1.0          |
| `voxformat_scale_y`           | Scale the vertices for voxelization on Y axis by the given factor                        | 1.0          |
| `voxformat_scale_z`           | Scale the vertices for voxelization on Z axis by the given factor                        | 1.0          |
| `voxformat_schematictype`     | The type of schematic format to use when saving schematics                               | mcedit2, worldedit, schematica |
| `voxformat_skinaddgroups`     | Add groups for body parts of Minecraft skins                                             | true/false   |
| `voxformat_skinapplytransform`| Apply transforms to Minecraft skins                                                      | true/false   |
| `voxformat_skinmergefaces`    | Merge faces of Minecraft skins into a single volume                                      | true/false   |
| `voxformat_texturepath`       | Additional search path for textures when importing mesh formats                          |              |
| `voxformat_transform_mesh`    | Apply the keyframe transform to the mesh                                                 | true/false   |
| `voxformat_voxcreategroups`   | Magicavoxel vox groups                                                                   | true/false   |
| `voxformat_voxcreatelayers`   | Magicavoxel vox layers                                                                   | true/false   |
| `voxformat_voxelizemode`      | `0` = high quality, `1` = faster and less memory                                         | 0/1          |
| `voxformat_vxllodhva`         | Load the HVA file for VXL animations                                                     | true/false   |
| `voxformat_vxlnormaltype`     | Normal type for VXL format - 2 (TS) or 4 (RedAlert2)                                     | 2/4          |
| `voxformat_withcolor`         | Export vertex colors                                                                     | true/false   |
| `voxformat_withmaterials`     | Export [material](Material.md) properties for formats that supports this                 | true/false   |
| `voxformat_withnormals`       | Export smoothed normals for cubic surface meshes (marching cubes always uses normals)    | true/false   |
| `voxformat_withtexcoords`     | Export texture coordinates                                                               | true/false   |

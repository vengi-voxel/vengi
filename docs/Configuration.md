# Configuration

## Variables

The engine can get configured by so called cvars (configuration variables). These variables can be modified from within
the game via key bindings, in-game console, the ui or scripts.

To get a list of supported cvars (they might differ from application to application), type the command `cvarlist` to the
in-game console (`CTRL+Tab` in the default binding) - or execute the application with `--help` (Example: `vengi-voxedit.exe --help`).

The variables can get their initial value from various sources. The highest order is the command line. If you specify it on
the command line, every other method will not be used. If the engine finds the cvar name in your environment variables, this
one will take precendence over the one the is found in the configuration file. Next is the configuration file - this one will
take precendence over the default settings that are specified in the code.

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

## Search paths

You can get a list of search paths by calling the particular application with the `--help` parameter on the command line. This will print a list of search paths. You can even add your own paths by setting the cvar `core_path`.

> For packagers it might be interesting to set the `PKGDATADIR` cmake variable.

## Commands

To get a list of supported commands (they might differ from application to application), type the command `cmdlist` to the
in-game console (`CTRL+Tab` in the default binding).

You can also get a list when doing `./vengi-app --help` on the command line.

## Key bindings

You can also modify or add key bindings to commands. Type `bindlist` to the console to get a list of the current active bindings
(and also here: they might differ from application to application). The command `bind` can be used to configure keybindings on-the-fly. These bindings are saved to a file on shutdown.

## Logging

You can either log via syslog (on unix) or to stdout (this might of course differ from platform to platform).

The log level is configured by the `core_loglevel` variable. The lower the value, the more you see. `1` is the highest log level
(trace), where 5 is the lowest log level (fatal error).

## General

To get a rough usage overview, you can start an application with `--help`. It will print out the commands and configuration variables
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

| Name                          | Description                                                                              |
| ----------------------------- | ---------------------------------------------------------------------------------------- |
| `voxformat_ambientocclusion`  | Don't export extra quads for ambient occlusion voxels                                    |
| `voxformat_mergequads`        | Merge similar quads to optimize the mesh                                                 |
| `voxformat_reusevertices`     | Reuse vertices or always create new ones                                                 |
| `voxformat_scale`             | Scale the vertices on all axis by the given factor                                       |
| `voxformat_scale_x`           | Scale the vertices on X axis by the given factor                                         |
| `voxformat_scale_y`           | Scale the vertices on Y axis by the given factor                                         |
| `voxformat_scale_z`           | Scale the vertices on Z axis by the given factor                                         |
| `voxformat_quads`             | Export to quads                                                                          |
| `voxformat_withcolor`         | Export vertex colors                                                                     |
| `voxformat_withtexcoords`     | Export texture coordinates                                                               |
| `voxformat_transform_mesh`    | Apply the keyframe transform to the mesh                                                 |
| `voxformat_marchingcubes`     | Use the marching cubes algorithm to produce the mesh                                     |
| `voxformat_createpalette`     | Setting this to false will use use the palette configured by `palette` cvar and use those colors as a target. This is mostly useful for meshes with either texture or vertex colors or when importing rgba colors. This is not used for palette based formats - but also for RGBA based formats. |
| `voxformat_fillhollow`        | Fill the inner parts of completely close objects                                         |
| `voxformat_scale`             | Scale the vertices on all axis by the given factor                                       |
| `voxformat_scale_x`           | Scale the vertices on X axis by the given factor                                         |
| `voxformat_scale_y`           | Scale the vertices on Y axis by the given factor                                         |
| `voxformat_scale_z`           | Scale the vertices on Z axis by the given factor                                         |
| `voxformat_voxel_mesh`        | Optimize import precision assuming that the mesh is composed of uniform voxels           |
| `voxformat_vxlnormaltype`     | Normal type for VXL format - 2 (TS) or 4 (RedAlert2)                                     |
| `voxformat_qbtpalettemode`    | Use palette mode in qubicle qbt export                                                   |
| `voxformat_qbtmergecompounds` | Merge compounds in qbt export                                                            |
| `voxformat_voxcreatelayers`   | Magicavoxel vox layers                                                                   |
| `voxformat_voxcreategroups`   | Magicavoxel vox groups                                                                   |
| `voxformat_merge`             | Merge all models into one object                                                         |
| `voxformat_rgbflattenfactor`  | To flatten the RGB colors when importing volumes (0-255) from RGBA or mesh based formats |
| `voxformat_qbsavelefthanded`  | Save qubicle format as left handed                                                       |
| `core_colorreduction`         | This can be used to tweak the color reduction by switching to a different algorithm. Possible values are `Octree`, `Wu`, `KMeans` and `MedianCut`. This is useful for mesh based formats or RGBA based formats like e.g. AceOfSpades vxl. |

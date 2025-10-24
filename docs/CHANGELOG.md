# Changelog

A more detailed changelog can be found on [github](https://github.com/vengi-voxel/vengi/commits/).

Join our [discord server](https://vengi-voxel.de/discord).

See [the documentation](https://vengi-voxel.github.io/vengi/) for further details.

Known [issues](https://github.com/vengi-voxel/vengi/issues?q=is%3Aissue+is%3Aopen+label%3Abug).

## 0.2.1 (2025-10-24)

General:

   - Minor bugfix release

## 0.2.0 (2025-10-24)

General:

   - Added missing ascii `fbx` header for saving and support texture coordinates and thumbnails for loading
   - Support `obj` point clouds
   - Added cvars `voxformat_imagesliceoffsetaxis` and `voxformat_imagesliceoffset` to import spritestacks via Aseprite `asprite` files
   - Fixed symlink handling on linux
   - Reduced memory usage
   - Optimized palette color mappings
   - Added `meshoptimizer` simplification support for mesh imports (cvar `voxformat_mesh_simplify`)
   - Improved split-by-objects for hollowed objects
   - Bump `vengi` format version
   - Improved a few error messages and added missing error handling to some parts
   - Optimized voxelization (subdividing triangles)
   - Added support for goxel `txt` file format
   - Added support for the veloren server chunk `dat` files
   - Allow to export all cvars as json
   - Optimized loading vengi files

VoxEdit:

   - Allow to animate point and camera nodes, too
   - Allow to follow an animated camera to fly through a scene
   - Fixed `resetcamera` when transforms are applied in edit mode
   - Shared voxel editing with client/server collaboration mode
   - Enable undo/redo for big volumes
   - Refactored scene rendering settings dialog
   - Improved shadow rendering
   - Support translations and lua scripts in the web version
   - Implemented basic physics that allows you to walk through a scene
   - Allow to move a node in the hierarchy but don't move it in the world

VoxConvert:

   - Transfer existing thumbnails between formats (if supported)
   - Allow isometric console output of a scene
   - A web version is available now, too

Thumbnailer:

   - Added isometric thumbnail support
   - Allow to set sun parameters

## 0.1.0 (2025-07-19)

General:

   - Fixed handling of invalid `jpeg` files
   - Added new normal palette from slab6
   - Reduced overall memory usage
   - Improved format detection for the file dialog
   - New lua script `shadow` to generate shadows for a scene
   - General performance improvements
   - Minecraft skin loading and saving

VoxEdit:

   - New single-non-contiguous voxel placing mode in the brushes
   - Added new view modes for Minecraft skins and Ace Of Spades

PalConvert:

   - Allow to use the built-in as well as lospec palettes from the command line, too

VoxConvert:

   - Added `--image` to voxconvert to print the loaded scene to the colored text console ([showcase](https://asciinema.org/a/724333))
   - Thumbnails for formats that support them can now be generated and added to the files

## 0.0.38 (2025-06-01)

General:

   - Fixed an off-by-one bug in the marching cubes extractor
   - Fixed outline rendering in scene mode
   - Fixed `vxl` slab5 loading
   - Support `binvox` version 2 and 3 (unofficial)
   - Allow to save all three versions of `binvox` files (cvar `voxformat_binvoxversion`)
   - Improved DirectStateAccess (DSA) support for OpenGL
   - Quite a few performance improvements
   - Support colors on apple and windows terminals, too
   - Added lua binding for genland algorithm. See the new `g_algorithm.genland` function
   - Allow to save scenes as heightmaps (`png`). See the new `voxformat_imagesavetype` cvar
   - Added support for writing Luanti/Minecraft `mts` files

VoxEdit:

   - Fixed directory import on Windows systems
   - Fixed missing ambient occlusion for the binary mesher mode
   - Fixed bloom rendering artifacts
   - Fixed material changes not being applied to the palette

## 0.0.37 (2025-05-12)

General:

   - Added new minecraft block to color mappings
   - Improved bash completion scripts
   - Fixed `vmax.zip` loading
   - Improved VoxelMax scene loading
   - Improved color mapping for minecraft blocks with prefixes
   - Fixed some dpi issues with a few dialog sizes
   - Added support for Avoyd material template (`avmt`) files
   - Fixed new binary mesher for negative region offsets
   - New lua scripts
   - Added lua script description

VoxEdit:

   - Added new dialog to show the minecraft block to color mappings
   - Extended palette function (white-balancing and constrast-stretching)
   - Fixed showing a few keybinding hints
   - Refactored the node inspector
   - Improved the camera panel
   - Allow to change the cvar `ve_regionsizes` from within the UI
   - Fixed crash in memento panel tooltip handling
   - Optimized lua script panel

PalConvert:

   - Print palette and color names

## 0.0.36 (2025-04-21)

General:

   - Fixed emit value handling for palettes
   - Fixed `KHR_materials_pbrSpecularGlossiness` handling for `gltf` exports
   - Added new lua script to create newel stairs
   - Fixed missing animation transfer from one scenegraph into other one
   - Updated/Added SDL3 support (both SDL2 and SDL3 are supported now)
   - Added Adobe ColorBook (`acb`) palette support
   - Fixed `xraw` palette issue with color slot 0
   - Added `schematic` write support (Minecraft)
   - Fixed Adobe Swatch Exchange (`ase`) palette loading issue
   - Use a faster binary-greedy-mesher
   - Optimizations
   - Fixed node merging with pivots
   - Allow to load other palette image dimensions than 1x256

VoxEdit:

   - Implemented selection invert
   - Added palette options to file dialog
   - Allow to execute palette actions on multiple colors
   - Allow to reduce multiple colors at once
   - Fixed missing font glyphs in some cases

PalConvert:

   - Added a new tool for dealing with palettes

## 0.0.35 (2025-02-09)

General:

   - Added support for BenVoxel `ben` and `ben.json` format
   - Added support for loading vertex colors for `3ds`
   - New cvar to define additional texture search paths for voxelization (`voxformat_texturepath`)
   - Fixed an error with loading magicavoxel files without layers
   - New lua script to remap colors
   - Prevent files from being opened twice (windows doesn't like this)
   - Fixed unicode related errors on windows for path handling
   - Fixed case-sensitivity in pattern matching. We couldn't find files that had upper-case extensions
   - Implemented symlink handling on windows
   - Added public domain Ace Of Spades map generator code
   - Added new cvar `palformat_maxsize` to control the max image size for palette quantization
   - Added new cvar `voxformat_imageheightmapminheight` to control the minimum size during a heightmap import
   - Fixed handling of `gltf` embedded images
   - Fixed `png` import palette handling
   - Fixed issues with `thing` format
   - Fixed issues with `vxl` format
   - Added support for loading quake `map` files (but this is still work-in-progress)
   - Added new blocks to `sment` StarMade palette
   - Added new lua script `flatten`
   - Fixed issue with auto selecting files in the file dialog if the options popup is visible
   - Added support for KenneyShape `kenshape` format
   - Added save support for Godot `escn` scene format

VoxConvert:

   - Removed the file type selection for source and target files in the ui
   - Added script support to the ui
   - `--usage` shows lua script details now
   - Removed `--slice` (see `png` format)
   - Allow to specify multiple `--script` parameters

VoxEdit:

   - Added the possibility to render a plane to the viewport for easier orientation
   - Show the normals in scene mode, too
   - Allow to edit models in-place in edit mode

## 0.0.34 (2024-11-14)

General:

   - Fixed invalid uv coordinate handling when importing meshes to voxelize them
   - Added support for paint.net palettes
   - Added support for Adobe Photoshop ACO palettes
   - Added support for `bbmodel` Blockbench format
   - Added support for `vxb` Sandbox Block format
   - Fixed a few errors in the lua bindings of the `g_import` global
   - Extended the lua api (`g_sys`, `g_io`)
   - Fixed nameless, embedded textures in `glb`
   - Fixed off-by-one in `voxformat_voxelizemode` `1`
   - Fixed missing axis flip for `md2` scale and translate value imports
   - Added support for Quake1 `mdl` files
   - Added support for exporting and importing to and from `png` slices
   - Added support for Autodesk `3ds` files
   - Support for loading normals for voxelization (mesh formats) and for voxel formats (`vxl`)
   - Fixed `vxl` saving for negative coordinates
   - Split file dialog options into a new separated dialog
   - Added new lua scripts (game of life, mandelbulb, smooth)
   - Added support for Pixelorama palette

VoxConvert:

   - Removed `--image-as-XXX` parameters (now part of the `png` format)
   - Removed `--colored-heightmap` (this is auto-detected in the `png` format now)

VoxEdit:

   - Allow to change the local directory for the asset panel
   - Add more online sources to the asset panel
   - Added support to apply a checkboard pattern to your voxels just for rendering
   - Improved handling for max allowed voxels
   - Disable undo/redo for model changes if you exceed a max suggested model size
   - Disable autosaves for model changes if you exceed a max suggested model size
   - New brush type to project textures into sub areas or on surfaces
   - Added an FPS option for the animation timeline
   - Added normal palette panel
   - Fixed missing memento group for merging nodes
   - Improved undo/redo for lua script changes on the scenegraph
   - Added a slice mode to only show a particular height level of your volume
   - Don't place invalid colored voxels in group lock mode
   - Render forward direction in the viewport

## 0.0.33 (2024-08-05)

General:

   - Fixed invalid log levels
   - Added a new node type for named points
   - Improved `3zh` format support
   - Added support for `ase` and `aseprite` format
   - Added cvar `voxformat_emptypaletteindex`
   - Create *walk* and *jump* animations with the new `animate.lua` script
   - New lua script and bindings (`imageasvolume.lua`) for importing 2d images with depth
   - Added `v3b` format support (compressed `v3a` file)
   - Translation improvements
   - Fixed mesh extraction at chunk borders in some situations
   - Added support for libjpeg to speed up save times (and thus avi creation times)
   - Improved Windows file attribute handling
   - `writeString` lua stream binding has a new default value - check the docs

VoxConvert:

   - Added a basic UI for voxconvert
   - Removed `--dump` and added `--json <full>` to generate a scene graph structure

VoxBrowser:

   - Removed standalone application - it's integrated in voxedit now

VoxEdit:

   - Improved brush support
   - Fixed loading voxel collection assets
   - Gradient paint brush mode
   - Fixed invalid voxel erasing in plane brush
   - Extrude with preview (plane brush)
   - Fixed a few palette panel issues when re-ordering colors
   - Improved the workflow to import a new palette

## 0.0.32 (2024-05-29)

General:

   - Add meshoptimizer support (new cvar `voxformat_optimize`)
   - Fixed missing region shift for `vxl` (Command and Conquer)
   - Fixed MagicaVoxel `vox` file loading for files that don't contain instances, but only models
   - Added `vxc` thumbnail support
   - Fixed a translation related crash for all apps on windows

VoxEdit:

   - Converted the text-voxel-rendering into a new brush
   - Fixed undo node transform changes as first action

## 0.0.31 (2024-05-09)

General:

   - Fixed memory leak in `fbx` loading
   - Added support for starmade templates `smtpl` format
   - Fixed merging of nodes when pivots are used
   - Added support for `litematic` Minecraft
   - Extended supported Minecraft materials
   - Extended lua bindings for key frames and transforms
   - Extended lua bindings for http requests
   - Fixed face culling for negative scale values
   - Added command and cvar dialogs to the help menus
   - Added cubzh v5 (older version) support (`3zh`)
   - Added support for writing uncompressed qubicle `qb` files
   - Fixed qubicle `qb` version number
   - Added basic support for rooms.xyz `thing` file support
   - Optimized file dialog for large directories
   - Added translation support (`po` files)
   - Fixed error in up-scaling volumes
   - Fixed x-axis flip for cubzh `3zh` files
   - Added cvar `voxformat_rgbweightedaverage` to control the color contributions for a triangle
   - Added the ability to upload crash dumps
   - Improved support for reference nodes in a lot of formats
   - Improved pivot support in a lot of formats

VoxConvert:

   - Added zip archive support for `--input`
   - Added the ability to export into slices with `--slice` when `--output` is a png file

VoxEdit:

   - Added support for browsing the remote voxel collections to the asset panel
   - Added `modelmerge all` parameter to merge all nodes at once
   - Added `presentation` command to cycle through all models in a scene
   - Improved position panel to edit transforms
   - Improved start-up times
   - Pasting from a different palette will search the closest colors
   - Fixed off-by-one error in `ve_regionsizes` cvar handling
   - Re-enabled the yocto-pathtracer panels again
   - Moved some ui elements into other panels
   - Improved stamp brush features
   - Fixed brush regression in combination with the override mode
   - Added pathfinder brush preview

Thumbnailer:

   - Fixed logic error in skipping camera nodes

Packaging:

   - Renamed AppStream package to `io.github.vengi_voxel.vengi.voxedit` for DBUS compatibility

## 0.0.30 (2024-03-23)

General:

   - Fixed material saving and loading for vengi format
   - Fixed off-by-one in palette material property handling
   - Fixed extrude on region boundaries
   - Added dialogs to recover from previous crash by resetting all config values

VoxBrowser:

   - Optimizations for huge voxel collections
   - Fixed some download url assembling errors for some online collections

VoxEdit:

   - Fixed segfault when activating simplified mode
   - Fixed spanning AABB
   - Fixed crash on deleting reference nodes after duplication
   - Fixed pivot handling issue for reference nodes
   - Spurious reference nodes were visible in edit mode
   - Fixed a few memento state handling errors in terms of key frames and pivot points
   - Fixed memento state handling errors for reference nodes
   - Disable a few of the new panels in simple mode again
   - Improved scene mode node selection

## 0.0.29 (2024-03-15)

General:

   - Expose `hollow()` to lua bindings
   - Add lua sanity checker to the build pipelines
   - Fixed randomness in lua script `similarcolor.lua`
   - Fixed error in voxelizer handling off-by-one cases in negative vertex coordinates
   - Fixed `vxl/hva` loading and saving
   - Fixed missing `pcubes` write support
   - Support palette [materials](Material.md)
   - Optimized qubicle `QB` color quantization
   - Import magicavoxel materials
   - Improved `GLTF` material support
   - Made the smooth normals for the cubic mesh extractor optional (`voxformat_withnormals`)
   - Improved texture coordinates for mesh exports
   - Improved bash completion script creation
   - Improved scene camera centering
   - Fixed Anima Toon (`scn`) volume loading support
   - Palette optimizations
   - Improved StarMade `sment`, `smd2` and `smd3` support

VoxBrowser:

   - New tool to browse voxel collections

VoxConvert:

   - Added `--print-formats` parameter to print all formats as json the ease the parsing for third party tools
   - Support multiple `--output` parameters to write one (or more) input files into different formats in one step

VoxEdit:

   - Auto create key frames when transform is modified via gizmo, too
   - Fixed an error in interpolating between the key frames
   - Fixed an error while trying to remove unused colors from a re-sorted palette
   - Fixed missing outline for dark voxels
   - Update paint features
   - Added more features to the palette panel (e.g. duplicating and removing colors)
   - Allow to bind the left- and right-scrolling mouse wheel
   - Improved draw color brush darken/brighten to create new colors if needed (and possible)
   - Fixed voxel placing on using the camera view manipulator
   - Fixed grid culling for orthographic projections
   - Selection in ortho fixed side view spans the whole size of the volume
   - Updated key bindings
   - Added popup for renaming a node
   - Fixed a few palette panel issues for sorting and re-ordering
   - Improved grid rendering
   - Added new models to the new-scene-dialog

Thumbnailer:

   - Don't render camera frustums for the thumbnails
   - Allow to change the camera mode (top, left, right, ...)

## 0.0.28 (2024-01-17)

> moved the github project into an organization named **vengi-voxel** - the url changed to [https://github.com/vengi-voxel/vengi](https://github.com/vengi-voxel/vengi).

General:

   - Improved SLAB5/6 support of `kvx`, `kv6`, `kfa` and `vox`
   - Don't fill the inner voxels of the SLAB5/6 models anymore
   - Improved palette support for plane and volume import
   - Fixed an error in de-duplicating Magicavoxel models
   - Allow one to save/convert only visible nodes
   - `ply` format got support for converting polygons into triangles
   - Show the full application name as window title
   - Improved esc handling for menus and popups
   - Added support for particube `pcubes` and `3zh` support
   - Added support for Photoshop `ase` palette
   - Added support for Paintshop Pro (JASC) `pal` palette
   - Extended support for the Gimp `gpl` palette format to support the Aseprite alpha channel extension
   - Added support to collect anonymous metrics (disabled by default)
   - Added support to align all models in a scene - useful for rendering all models for an overview
   - Extended lua bindings
   - Added automatic bash and zsh completion generation for all apps. Just use e.g. `vengi-voxconvert --completion bash` (or `zsh`)
   - Calculate normals for the cubic mesh extractor, too
   - Added support for cubzh `b64` maps
   - Fixed endian issue on big endian machines in `gltf` loading
   - Fixed non-flipped uv coordinates for some `gltf` files

VoxConvert:

   - Improved `--export-models` to work with the output format given by `--output`
   - Added `--filter-property` to filter by node property `key<:value>`

VoxEdit:

   - Added alternative split-object implementation and expose it to the editor tool panel
   - Fixed split dialog appearing too late
   - Added shader for marching cubes
   - Fixed selection handling
   - Added tip of the day dialog
   - Resize to selection
   - Fixed palette panel issue if you changed the order of colors
   - Added more templates to the new-scene-dialog
   - Made the new-scene-dialog scrollable
   - Allow one to change the transform of a node with updating the children
   - Fixed a few crashes
   - Improved gizmo settings panel
   - Added the console next to the animation timeline panel
   - Improved color themes (especially the light theme)
   - Only show the brush panel if in edit mode
   - Converted the script brush back to a normal panel
   - Automatically reference all child nodes when creating a new reference
   - Allow one to switch between cubes and marching cubes rendering in the editor
   - Added `clear` and `fill` commands
   - Preview for the line brush
   - Allow to render the bones

Thumbnailer:

   - Add a few more command line options to control the camera

## 0.0.27 (2023-09-19)

Breaking changes:

   - Renamed globals for lua scripts. Added a `g_` prefix to them. You have to adopt your scripts
     to work with the latest version with vengi (if you use them):
     * `scenegraph` is now `g_scenegraph`
     * `palettemgr` is now `g_palette`
     * `noise` is now `g_noise`
     * `cmd` is now `g_cmd`
     * `var` is now `g_var`
     * `XvecX` is now `g_XvecX` (`ivec3` is for example `g_ivec3`)

General:

   - Added cvar `voxformat_pointcloudsize` for point cloud formats
   - Added Polygon File Format (`ply`) mesh and point cloud support (ascii and binary)
   - Fixed regression with `GLTF` exports
   - Added a new lua script to slice a node into smaller pieces
   - Expose shape generators to lua
   - Fixed color intensity handling for `kvx` files
   - Added write support for `kvx` format (used in e.g. voxel doom and eduke3d)
   - Added support for Voxel3D `v3a` format

VoxEdit:

   - Fixed regression about not rendering the shape volumes anymore
   - Implemented scene graph panel drag and drop popup
   - Added brush support and new editing features
   - Fixed spurious crashes for windows

## 0.0.26 (2023-08-13)

General:

   - Fixed pivot handling in `VXR` format
   - Allow one to export the `GLTF` vertex colors as byte values
   - Added NeuQuant color quantization algorithm
   - Optimized `GLTF` and `MD2` import
   - Fixed Quake1 texture handling
   - General optimizations
   - Added a new voxelization algorithm and a cvar to use it (set `voxformat_voxelizemode` to 1)
   - Fixed issues with importing images with a depth-map
   - Fixed regression for reading minecraft voxel data
   - Added support for loading Lospec palettes

VoxEdit:

   - Improved shape handling a lot
   - Added (disabled) pathtracer support (Yocto/GL)
   - Fixed rendering order of overlapping bounding boxes for the active node
   - Added warning popup to split volumes into smaller pieces

## 0.0.25 (2023-07-28)

General:

   - Fixed invalid x axis handling for Sandbox `VXM` format
   - Fixed pivot handling in `VXR`/`VXA` format
   - Support model references to build a complex scene
   - Extended `GLTF` support to animation import and export as well as normal export
   - Fixed missing base color support for `GLTF`
   - Support for some parts of VoxelMax format
   - Fixed Sandbox `VXA` v3 support
   - Fixed volume rotation issues
   - Fixed volume merging issues
   - Added Quake2 Model (`md2`) support
   - Removed cvar `voxformat_marchingcubes` and replaced with `voxel_meshmode` (set to `1` to use marching cubes)
   - Added new lua script `gradient.lua`
   - Improved `csm` and `nvm` format support
   - Added lua script for generating mazes
   - Added the ability to easily scale a volume up
   - Unified naming of commands and parameters (`layer` is `model` now, ...)
   - Added VoxelBuilder (`vbx`) support
   - Fixed missing group transform handling for MagicaVoxel `vox` files
   - Added support for MagicaVoxel `xraw` format
   - Added alpha support for MagicaVoxel materials
   - Added support for loading Sandbox `VXM` v3
   - Added new lua script and extended lua api to replace palettes, resize volumes and move voxels

VoxConvert:

   - Fixed `--input` handling if the value is a directory
   - Added option `--wildcard` to specify a wildcard in cases where the `--input` value is a directory
   - Added `--surface-only` parameter that removes any non surface voxel.
   - Added `--dump-mesh` to also show mesh details (like vertex count).

VoxEdit:

   - Allow one to export selections only
   - Started to support different keymaps (blender, qubicle, magicavoxel and vengi own)
   - Added support for multiple animations in one scene
   - Allow one to duplicate all scene graph nodes
   - (Re-)implemented WASD controls in camera eye mode
   - Fixed copy&paste errors with multiple selections
   - Updated new scene dialog to include the templates
   - Improved font quality
   - Fixed a few crashes
   - Added hollow functionality to remove invisible voxels
   - Preview of modifier shapes in edit mode

Thumbnailer:

   - Improved camera placement
   - Fixed threading issue with the mesh extraction for the renderer that could lead to black thumbnails
   - Don't render cameras for the thumbnails

## 0.0.24 (2023-03-12)

General:

   - Support using `palette` cvar as a destination palette again by setting `voxformat_createpalette` to `false`
   - Added new image formats from SOIL2 (`DDS`, `PKM`, `PVR`)
   - Added `FBX` read support
   - Improved texture lookup for mesh formats
   - Added multiple color reduction algorithms and expose them to the user by the cvar `core_colorreduction`
   - Fixed several issues with AceOfSpades `vxl` format - switched to libvxl for loading and saving
   - Fixed splitting volumes even if not needed (off-by-one error)
   - Fixed multi-monitor support
   - Fixed colors in the tree generator
   - Improved the key binding handling and made it more flexible
   - Added support for loading minetest `mts` files
   - Fixed Command&Conquer `vxl` format writing issue
   - Added own format with the extension `vengi`
   - Added basic alpha support
   - Fixed saving the keybindings with multi click
   - Improved keybinding and ui setting saving (added a version)
   - Fixed invalid transform on re-parenting a node
   - Refactored the file dialog
   - Fixed issue in saving MagicaVoxel `vox` files under some special conditions
   - Implemented basic transparency support for voxels
   - Fixed invalid clamping for uv based pixel lookup (mesh imports)
   - Update renderer to only use uniform buffers
   - Fixed rendering issue for windows users
   - Fixed fullscreen issues for windows users
   - Extended lua script bindings to Allow one to render text as voxels
   - Support GLES3 rendering

VoxConvert:

   - Show supported palette and image formats in help screen (`--help`)

VoxEdit:

   - Disable animation window if there are no animations to show
   - Allow one to show the color picker in the palette panel
   - Reworked the modifiers panel
   - Made undo/redo more visible
   - Switched the camera modifier to the right side of the viewport
   - Fixed diffuse and ambient color settings mix-up
   - Merged layer and scene graph panel into one
   - Several gizmo related fixes and improvements
   - Added simple UI mode which removes some panels
   - Fixed a few memento (undo/redo) related issues
   - Added templates to the menu (`robo`, `chess`, `head` and `chr_knight`)
   - Allow one to control the amount of viewports
   - Allow one to define pre-defined region sizes (see `ve_regionsizes` cvar)
   - Some ui actions are only available in edit mode
   - Edit mode has support for the gizmo now - you can shift the volume boundaries
   - Highlight copy&paste volume region
   - Visually disable some buttons if they won't work in the current mode anyway
   - Don't execute actions for hidden nodes
   - New keybindings
   - Allow one to sort the palette colors by hue, saturation or brightness
   - Allow one to select and drag keyframes in the animation timeline
   - Export animations as `AVI`
   - Allow one to import a whole directory into a scene
   - Allow one to select all node types in the scene graph panel
   - Allow one to edit node properties
   - Added voxel cursor tooltips about the position in the volume
   - WASM/HTML5 port

## 0.0.23 (2022-12-17)

General:

   - Improved big endian support for voxel formats
   - Improved `VXL` format default palette support
   - Improved `QBCL` scene graph support
   - Improved voxelization vertex color support
   - Fixed `VOX` root node handling
   - Fixed `QBCL` and `GOX` thumbnail handling
   - Removed unused code
   - Added support for `VXA` v3
   - (Re-)added support for marching cubes
   - Fixed a bug in Ace of Spades `VXL` loading

VoxEdit:

   - Added support for embedding screenshots in formats that support it
   - Allow one to export palettes
   - Allow one to change color intensity of the whole palette
   - Allow one to voxelize text
   - Fixed orthographic cameras
   - Massive performance increase when using multiple viewports
   - More than 10 times gpu memory reduction
   - Render the camera nodes in scene mode

Thumbnailer:

   - Added support for turntable rendering

## 0.0.22 (2022-10-31)

General:

   - Improved `GLTF` format support
   - Improved `VXL` format support
   - Improved Qubicle `QB` support
   - Fixed block id parsing for StarMade voxel models
   - Major improvements in scene graph transform handling
   - Improved voxelization of meshes with voxels
   - Added `kv6` write support
   - Added slab6 `vox` write support
   - Fixed saving black colors for cubeworld
   - Fixed saving palette index 0 for binvox
   - Fixed Sandbox `VXM` palette issue
   - Fixed `QBCL` saving
   - Improved `qbt` scene graph support
   - Improved `vox` saving with multiple palettes
   - Improved the file dialog size and special dir handling
   - Improved dark mode support
   - Improved palette support for some formats

Packaging:

   - There is a snap package available now (`io.github.vengi-voxel.vengi.voxedit`)

VoxEdit:

   - Fixed layer color selection if you have multiple layers
   - Fixed moving nodes in the scene graph panel
   - Fixed transforms in scene graph mode (translation, rotation)
   - Allow one to add group and camera nodes
   - Don't just quit the application if you have unsaved data in your scene

## 0.0.21 (2022-09-05)

General:

   - Added support for minecraft 1.13 region files
   - Added support for loading minecraft `level.dat` (only with supported region files)
   - Added support for WorldEdit `schematics`
   - Added support for Minecraft `nbt` files
   - Added support for StarMade voxel models
   - Added support for Quake1 and UFO:Alien Invasion
   - Reduced memory footprint for voxelizing huge meshes
   - Support wrap mode texture settings for gltf voxelization
   - Improved sanity check for Qubicle Binary format support
   - Fixed texture lookup error in `gltf` voxelization
   - Extended lua vector bindings and Allow one to import heightmaps
   - Improved the file dialog filters
   - Added new lua scripts and extended lua integration in voxconvert
   - Added support for RGB (`pal`) and Gimp (`gpl`) palette loading
   - Improved the Command & Conquer `VXL` format support

VoxEdit:

   - Fixed resetting the camera in eye mode
   - New asset panel
   - Place images, models and colors via drag and drop into the scene
   - Extended scene mode modifiers to allow resizing the volumes (double click in scene mode)
   - Fixed updating the locked axis plane on change
   - Fixed scene graph node panel width
   - Fixed mesh extraction on chunk boundaries
   - Fixed new volume dialog input handling
   - Fixed cursor being invisible with bloom disabled
   - Fixed cursor face being on the wrong side of the voxel at the volume edges
   - Fixed last opened files with spaces in their names
   - Fixed loading files from command line again
   - Allow one to select scene graph node from the animation timeline
   - Fixed deleting key frames
   - Improved adding new key frames

## 0.0.20 (2022-06-14)

General:

   - Added support for minecraft `schematic`
   - Refactored and extended the lua script integration
   - Implemented applying depth/height map to a 2d plane
   - Added support for new magicavoxel format (animations)
   - Preserve node hierarchy when saving `vxr`
   - `GLTF` voxelization
   - Allow one to enable certain renderer features
   - Expose more noise functions to the lua scripts
   - Expose more volume functions to the lua scripts
   - Allow one to delete voxels from within a lua script
   - Improved splitting of volumes (target volume size)
   - Expose more region functions to the lua scripts
   - Added more lua example scripts
   - Improved color sampling for voxelization
   - Started to support different palettes in one scene
   - Fixed `vxc` support
   - Load the palette from the source file
   - Fixed `vxm` file path when saving `vxr`
   - Save `vxmc` v12 now
   - Changed default ambient color and gamma values
   - Improved osx dmg file and app bundles

VoxEdit:

   - Fixed start problems on some systems with multisampled framebuffers
   - Allow one to drag and drop colors from the palette
   - Change between the edit and scene mode is now bound to `tab`
   - Updated imgizmo to support clicking the view cube
   - Cursor is no volume anymore but a plane
   - Implemented plane filling
   - Added extrude feature
   - Allow one to place a single voxel
   - Fixed keyboard input errors that made the ui unusable
   - Don't reload the last opened file with every start

VoxConvert:

   - Extended `--dump` to also show the key frames and the voxel count
   - Removed `--src-palette` (src palette is always used)

## 0.0.19 (2022-03-27)

General:

   - Replaced minecraft support with own implementation
   - Added support for Sandbox `VXA` format (via `VXR`) and improved `VXR`
   - Allow one to change the ui colors via cvar (`ui_style`)
   - Added bloom render support for vox and vxm
   - Added support for loading key frames if the format supports it
   - Improved apple support in file dialog
   - The palette handling was refactored
   - Allow one to save the MATL chunk in magicavoxel `vox` files
   - Ability to scale exported mesh with different values for each axis
   - Added stl voxelization support
   - Allow one to modify the camera zoom min/max values
   - Allow one to load different sizes for AoS `VXL` files
   - Lerp the camera zooming
   - Added support for `GLTF` export
   - Added experimental export support for `FBX` ascii
   - Increased the max scene graph model nodes from 256 to 1024

VoxEdit:

   - Added new command to fill hollows in models
   - Fixed escape key not closing the dialogs
   - Added support for drag and drop the nodes of the scene graph
   - Scene graph rendering improved
   - Removed noise panel (use the lua scripts for noise support)
   - Fixed a lot of undo/redo cases and improved the test cases a lot
   - Fixed viewport screenshot creation (now also bound to F5)
   - Added dialog to configure the mesh and voxel format settings for loading/saving
   - Improved the palette panel
   - Improved the gizmo for translation and rotation
   - Open in scene mode as default

VoxConvert:

   - Added `--image-as-plane` and `--image-as-heightmap` parameters
   - Allow one to create a palette from input files

## 0.0.18 (2022-02-12)

> renamed the github project to **vengi** - the url changed to [https://github.com/vengi-voxel/vengi](https://github.com/vengi-voxel/vengi).

Build:

   - Removed own cmake unity-build implementation
   - Fixed build when `GAMES` was set to `OFF`

General:

   - Extended qbcl format support
   - Fixed color conversion issue when importing palettes from voxel models
   - Voxelization of `obj` meshes now also fills the inner parts of the mesh with voxels
   - Fixed magicavoxel pivot issue (sometimes wrong positions)
   - Added support for sandbox `vxc` format
   - Added support for sandbox `vxt` format
   - Added new example lua scripts

VoxConvert:

   - `--input` can now also handle directories

VoxEdit:

   - Added context actions to scene graph panel
   - Fixed mouse input issues in fullscreen mode
   - Fixed script editor placement

## 0.0.17 (2022-01-23)

General:

   - Fixed relative path handling for registered paths
   - Stop event loop if window is minimized (reduce cpu usage)
   - Support scene graphs in the voxel formats
   - Fixed a few issues with the magicavoxel vox format (switched to `ogt_vox`)
   - Load properties from supported voxel formats (`vxr`, `vox`, `gox`)
   - Added support for loading minecraft region files (used enkimi)
   - Fixed `vxm` pivot and black color issue
   - Added `obj` voxelization
   - Improved `obj` export
   - Improved file dialog

VoxConvert:

   - Added `--crop` parameter that reduces the volumes to their real voxel sizes
   - Added `--split` option to cut volumes into smaller pieces
   - Added `--export-models` to export all the models of a scene into single files
   - Added `--dump` to dump the scene graph of the input file
   - Added `--resize` to resize the volumes by the given x, y and z values

VoxEdit:

   - Fixed torus shape
   - Added scene graph panel
   - Fixed an issue that delayed the start by a few seconds

## 0.0.16 (2021-12-27)

General:

   - Fixed magicavoxel vox file saving
   - Added support for old magicavoxel (pre RIFF) format
   - Fixed bugs in `binvox` support
   - Fixed save dir for `vxm` files when saving `vxr`
   - Save `vxm` v5 (with included pivot)
   - Support bigger volumes for magicavoxel files

VoxConvert:

   - Fixed `--force` handling for target files
   - Allow one to operate on multiple input files
   - Added `--translate` command line option
   - Added `--pivot` command line option
   - nippon palette is not loaded if `--src-palette` is used and it's no hard error anymore if this fails

VoxEdit:

   - Add recently used files to the ui

## 0.0.15 (2021-12-18)

General:

   - Fixed missing `vxm` (v4) saving support
   - Fixed missing palette value for `vxm` saving
   - Added support for loading only the palettes
   - Added support for goxel `gox` file format
   - Added support for sproxel `csv` file format
   - Added support for a lot more image formats
   - Improved lod creation for thin surface voxels
   - Fixed `vxr` v9 load support
   - Added support for writing `vxr` files

VoxConvert:

   - Added option to keep the input file palette and don't perform quantization
   - Allow one to export the palette to `png`
   - Allow one to generate models from heightmap images
   - Allow one to run lua scripts to modify volumes
   - Allow one to export or convert only single layers (`--filter`)
   - Allow one to mirror and rotate the volumes

Thumbnailer:

   - Try to use the built-in palette for models

VoxEdit:

   - Allow one to import palettes from volume formats, too
   - Implemented camera panning
   - Added more layer merge functions

## 0.0.14 (2021-11-21)

General:

   - License for our own voxel models is now CC-BY-SA
   - Support loading just the thumbnails from voxel formats
   - Support bigger volume sizes for a few formats
   - Don't pollute the home directory with build dir settings
   - Fixed gamma handling in shaders
   - Added bookmark support to the ui dialog

Thumbnailer:

   - Added `qbcl` thumbnail support

VoxEdit:

   - Render the inactive layer in grayscale mode

## 0.0.13 (2021-10-29)

General:

   - Logfile support added
   - Fixed windows DLL handling for animation hot reloading

UI:

   - Fixed log notifications taking away the focus from the current widget

VoxEdit:

   - Fixed windows OpenGL error while rendering the viewport

## 0.0.12 (2021-10-26)

General:

   - Fixed a few windows compilation issues
   - Fixed issues in the automated build pipelines to produce windows binaries

## 0.0.11 (2021-10-25)

General:

   - Added url command
   - Reduced memory allocations per frame
   - Added key bindings dialog
   - Added notifications for warnings and errors in the ui
   - Fixed Sandbox Voxedit `VXM` v12 loading and added saving support
   - Fixed MagicaVoxel vox file rotation handling

VoxEdit:

   - Removed old ui and switched to dearimgui
   - Added lua script editor
   - Added noise api support to the lua scripts

## 0.0.10 (2021-09-19)

General:

   - Added `--version` and `-v` commandline option to show the current version of each application
   - Fixed texture coordinate indices for multi layer obj exports
   - Improved magicavoxel transform support for some models
   - Fixed magicavoxel x-axis handling
   - Support newer versions of `vxm` and `vxr`
   - Fixed bug in file dialog which prevents you to delete characters #77

VoxEdit:

   - Improved scene edit mode
   - Progress on the ui conversion to dearimgui

Tools:

   - Rewrote the ai debugger

## 0.0.9 (2020-10-03)

General:

   - Fixed obj texcoord export: Sampling the borders of the texel now
   - Added multi object support to obj export

## 0.0.8 (2020-09-30)

General:

   - Added obj and ply export support
   - Restructured the documentation
   - Improved font support for imgui ui

Backend:

   - Reworked ai debugging network protocol
   - Optimized behaviour tree filters

## 0.0.7 (2020-09-15)

General:

   - Fixed wrong-name-for-symlinks shown
   - Added support for writing qef files
   - Added lua script interface to generate voxels
   - Added stacktrace support for windows
   - Refactored module structure (split app and core)
   - Optimized character animations
   - Hot reload character animation C++ source changes in debug builds
   - Added quaternion lua support
   - Updated external dependencies
   - Refactored lua bindings
   - Support Chronovox-Studio files (`csm`)
   - Support Nick's Voxel Model files (`nvm`)
   - Support more versions of the `vxm` format

VoxEdit:

   - Converted some voxel generation functions to lua
   - Implemented new voxel generator scripts

## 0.0.6 (2020-08-02)

General:

   - Fixed gamma cvar usage
   - Enable vsync by default
   - Updated external dependencies
   - Activated OpenCL in a few tools
   - Added symlink support to virtual filesystem

VoxEdit:

   - Fixed loading palette lua script with material definitions
   - Fixed error in resetting mirror axis
   - Fixed noise generation
   - Reduced palette widget size
   - Fixed palette widget being invisible on some dpi scales

## 0.0.5 (2020-07-26)

Client:

   - Fixed movement

Server:

   - Fixed visibility check
   - Fixed segfault while removing npcs

VoxEdit:

   - Started to add scene mode edit support (move volumes)

VoxConvert:

   - Support different palette files (cvar `palette`)
   - Support writing outside the registered application paths
   - Allow one to overwrite existing files

General:

   - Switched to `qb` as default volume format
   - Improved scene graph support for Magicavoxel `vox` files
   - Fixed invisible voxels for `qb` and `qbt` (Qubicle) volume format
   - Support automatic loading different volume formats for assets
   - Support Command&Conquer `vxl` files
   - Support Ace of Spades map files (`vxl`)
   - Support Qubicle exchange format (`qef`)
   - Perform mesh extraction in dedicated threads for simple volume rendering
   - Improved gizmo rendering and translation support
   - Fixed memory leaks on shutdown
   - Improved profiling support via tracy

## 0.0.4 (2020-06-07)

General:

   - Added support for writing `binvox` files
   - Added support for reading `kvx` (Build-Engine) and `kv6` (SLAB6) voxel volumes
   - Performed some AFL hardening on voxel format code
   - Don't execute keybindings if the console is active
   - Added basic shader storage buffer support
   - Reduced voxel vertex size from 16 to 8 bytes
   - Apply checkerboard pattern to water surface
   - Improved tracy profiling support
   - A few highdpi fixes

Server:

   - Allow one to specify the database port
   - Fixed loading database chunks

VoxEdit:

   - Added `scale` console command to produce LODs

VoxConvert:

   - Added ability to merge all layers into one

## 0.0.3 (2020-05-17)

Assets:

   - Added music tracks
   - Updated and added some new voxel models

VoxEdit:

   - Made some commands available to the ui
   - Tweak `thicken` command
   - Updated default tree generation ui values
   - Save layers to all supported formats
   - Fixed tree generation issue for some tree types
   - Changed default reference position to be at the center bottom
   - Reduced max supported volume size

General:

   - Print stacktraces on asserts
   - Improved tree generation (mainly used in voxedit)
   - Fixed a few asserts in debug mode for the microsoft stl
   - Added debian package support
   - Fixed a few undefined behaviour issues and integer overflows that could lead to problems
   - Reorganized some modules to speed up compilation and linking times
   - Improved audio support
   - Fixed timing issues
   - Fixed invalid GL states after deleting objects

VoxConvert:

   - Added a new tool to convert different voxel volumes between supported formats
     Currently supported are cub (CubeWorld), vox (MagicaVoxel), vxm (VoxEdit Sandbox), binvox
     and qb/qbt (Qubicle)

Client:

   - Added footstep and ambience sounds

## 0.0.2 (2020-05-06)

VoxEdit:

   - Static linked VC++ Runtime
   - Extract voxels by color into own layers
   - Updated tree and noise windows
   - Implemented `thicken` console command
   - Escape abort modifier action
   - Added L-System panel

General:

   - Fixed `binvox` header parsing
   - Improved compilation speed
   - Fixed compile errors with locally installed glm 0.9.9
   - Fixed setup-documentation errors
   - Fixed shader pipeline rebuilds if included shader files were modified
   - Improved palm tree generator
   - Optimized mesh extraction for the world (streaming volumes)
   - Added new voxel models
   - (Re-)added Tracy profiler support and removed own imgui-based implementation
   - Fixed writing of key bindings
   - Improved compile speed and further removed the STL from a lot of places
   - Updated all dependencies to their latest version

Server/Client:

   - Added DBChunkPersister
   - Built-in HTTP server to download the chunks
   - Replaced ui for the client

Voxel rendering:

   - Implemented reflection for water surfaces
   - Apply checkerboard pattern to voxel surfaces
   - Up-scaling effect for new voxel chunks while they pop in
   - Optimized rendering by not using one giant vbo


## 0.0.1 "Initial Release" (2020-02-08)

VoxEdit:

   - initial release

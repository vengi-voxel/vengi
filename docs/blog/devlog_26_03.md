# March 2026

## A new contributor

There is another new contributor: [Tudi](https://github.com/Tudi)

Tudi already made quite a few quality-of-life improvements and added a lot of new features - new selection modes, new brushes, visual masking, a global clipboard, and more. Welcome Tudi and thanks a lot for the contributions.

## New selection modes

Tudi contributed several new selection modes that make working with voxels a lot more ergonomic.

### Configurable selection tint color

Selected voxels were previously only indicated by a subtle edge darkening effect. On gray or similarly-colored models this was nearly invisible. Selected voxels now receive a full-face color tint (default: green at 40% blend), making selections clearly visible regardless of model color. The tint color is configurable via Options → Rendering.

![selection tint](https://github.com/user-attachments/assets/07d36f65-3413-4c3f-a7c2-071ac9e10e5b#gallery)

### Box3D select mode

A new Box3D select mode lets you drag a 3D box with the select brush to mark all solid voxels inside the region. When active, the brush panel shows per-axis min/max sliders with fine-adjustment buttons and axis-colored labels. An XYZ gizmo at the AABB corner closest to the camera provides spatial orientation feedback.

![box3d select](https://github.com/user-attachments/assets/7a7172a2-3c72-4d88-8ee5-0a93ac82d5fb#gallery)

### FlatSurface select mode

FlatSurface selection flood-fills all connected solid voxels that share the same exposed face as the clicked voxel - useful for selecting floor tiles, walls, or any copalnar surface. An accepted deviation slider lets the fill cross height steps of +/- x voxels along the face-normal, so stairs and ramps can be selected in one click.

![flat surface select](https://github.com/user-attachments/assets/f1ce72f9-3597-4de0-8b44-9da0d2267fd7#gallery)

### Circle and Slope select modes

Two more selection modes arrived: Circle select lets you click-drag to define a circular (or elliptical) selection on a surface with adjustable radii, depth, and an optional 3D ellipsoid toggle. Slope select flood-fills connected surface voxels that fit a global slope plane computed via linear regression - great for selecting rooftops and angled terrain.

![circle slope select](https://github.com/user-attachments/assets/6c5d6f4d-c2b5-40bb-8691-e73bbb8c661d#gallery)

## Visual masking

When a selection exists on a model node, voxels outside the selection are now rendered dark gray while voxels inside render normally. This makes it immediately obvious which area you are working on. Brushes are constrained to the masked area - new voxels may only be placed adjacent to the selection boundary, and erase/override operations only affect selected voxels.

![visual masking](https://github.com/user-attachments/assets/2f71ed46-d667-42da-994a-e08cbd5ac7cc#gallery)

## New brushes

### Extrude brush

A new Extrude brush works on the current voxel selection to push geometry outward or carve it inward along a chosen face direction. Click a voxel face to set the extrusion direction, then adjust depth with +/- controls. Positive depth extrudes outwards, negative depth carves inwards. An optional "Fill sides" checkbox adds perpendicular side walls to keep the mesh manifold.

![extrude brush](https://github.com/user-attachments/assets/3bea6e7f-396a-481b-b139-7dfd0f25e608#gallery)

### Transform brush

The new Transform brush lets you manipulate selected voxels with four sub-modes (atm): Move, Shear, Scale, and Rotate. It also provides a preview mode that was also requested long time ago to modify the voxels first and only write them back on some commit event.

![transform brush](https://github.com/user-attachments/assets/aeb1a4c4-d9f0-45ef-85ad-cc1f91a77a3a#gallery)

### Circle shape (hollow cylinders)

A new Circle shape type was added to the shape brush that generates hollow circles (rings) for creating open tubes and pipes. Wall thickness is configurable via a UI slider.

![circle shape](https://github.com/user-attachments/assets/bc955cfc-76e2-4be6-82b8-102df3371590#gallery)

### Preview rendering for Extrude and Transform

Both the Extrude and Transform brushes now show a ghost overlay preview so users can see the result before committing. Negative extrusion (carving) still applies directly since air voxels can't render as ghost overlays. The extrude and transform brushes also gained support for gizmos in the viewport for directly manipulating the selection.

![preview rendering](https://github.com/user-attachments/assets/dc14e40e-4be3-421f-8e83-12fec4fa9da8#gallery)

### Auto-select for newly placed voxels

A new `ve_autoselect` option (on by default) automatically selects newly placed voxels after brush execution. This makes it easy to immediately transform, extrude, or mask newly placed geometry. The toggle is accessible from the Mask menu.

![auto-select](https://github.com/user-attachments/assets/a544496a-505c-4fc0-9040-8e4881cb2080#gallery)

## Global clipboard

A new globalcopy/globalpaste feature allows copying voxels between different VoxEdit instances. The selection is serialized to `globalclipboard.vengi` in the user's home directory and can be pasted into any running VoxEdit instance. Available via Edit → Global copy / Global paste.

![global clipboard](https://github.com/user-attachments/assets/17470888-a620-4985-a148-036e64d7997a#gallery)

## New format: MD3

I wanted to voxelize the World of Padman weapons and added format support for Quake 3's MD3 mesh format. The importer handles UV coordinates, compensates for fully transparent textures that reference shaders, and imports MD3 tags as point nodes.

## LUA script generators

A massive batch of new LUA generator scripts was added:

- **Trees**: red maple, black willow, bonsai, cherry tree, hawthorn, birch, balsam fir, butternut, pine & fir updates, palm update, and several more generic tree generators
- **Plants**: tulip, mushroom scripts, pot plants
- **Buildings**: houses, colosseum, desert rock formations
- **Decorations**: lego studs
- **L-system**: New `lsystem` LUA bindings for procedural generation

Scripts were also updated to use proper color lookup via hex colors instead of raw palette indices, and a new `hexcolor` parameter type was added along with a color picker widget in the script panel. The script panel also gained a searchable combo box for easier script selection. The hex color usage made the default colors palette independent and allows a script creator to provide better defaults.

## Session recording

VoxEdit can now record voxelart sessions by writing the network protocol messages to disk - you can share this file with others or use it to record scene creation later. The playback speed for these files can be configured before starting playback.

Also the recording of the viewport got animated GIF support. So there is AVI, MPEG and GIF now.

## Improved options dialog

The options dialog was overhauled and extracted into its own dedicated dialog. Configuration variables now have proper titles, descriptions, and types. Enum-style vars get combo box widgets, path vars get file pickers, and all settings are searchable.

## Sound module

A new sound module was added for playing WAV files. This is the foundation for future audio feedback in the editor - but right now it serves as a base to play a ping sound for the also new network chat feature. If you are using the network mode, you can type messages to connected users with an internal chat feature. And if someone pings you (`@name`), you will get a short sound notification.

## Camera improvements

Several camera-related commands were added and renamed for consistency: `camera_mouselook` for FPS-style navigation, `camera_reset`, `camera_mode`, `camera_projection`, and `move_up`/`move_down` commands bound to Q and E by default.

## Shell completion

PowerShell completion support was added alongside improvements to the existing zsh and bash completions. All completions now support cvar value completion.

## Panel improvements

The image and model asset panel was split into two dedicated panels and repositioned next to the animation timeline. The file panel was also split out separately.

## Bug fixes and improvement

- Removed a significant performance bottleneck in the binary mesher by eliminating the `prepareChunk` call
- Fixed a race condition in `SparseVolume`
- Improved OBJ voxelization performance by preventing buffer re-allocations
- Fixed mirror plane rendering and mirror axis not resetting the preview volume
- Fixed locked plane not updating properly
- Fixed undo with mirror plane resetting unrelated voxels
- Fixed GLTF animation rotation quaternion reading
- Fixed GLTF invalid mesh index for point nodes
- Fixed crash in very large vengi scenes
- Fixed loading a new scene not stopping a previously running animation
- Fixed several memento state handling issues
- Fixed undefined behaviour in `RawVolume::clear()` when building with msys2

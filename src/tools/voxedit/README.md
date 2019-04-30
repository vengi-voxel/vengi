# Purpose

Voxel editor for large scenes.

This editor was written to have a linux voxel editor with animation support for
my own engine and evolved into something that others might find useful, too.

# Features

* Large scene support
* Load vox, qbt, qb, vxm
* Save to vox, qbt, qb
* Exporting to a lot of formats (dae, obj, fbx, gltf, ...)
* Auto-saving
* Console to script your modeling (CTRL+TAB cmdlist)
* Key bindings via cfg file
* Four viewport mode
* Grid resolution
* Up to 32 layers
* Mirror mode
* Auto cropping volumes
* Auto generate content like trees or noise volumes
* Import heightmaps
* Undo/Redo
* Custom color palettes
* Reference positions to rotate around or create auto-generated structures at
* Viewport screenshots
* Import and voxelize dae, obj, fbx and a lot more formats
* Modelling with the left and right mouse buttons
* Modelling without the mouse - just via shortcuts and keyboard
* Animation support
* Layer support
* Custom key bindings
* Show commands of actions (to configure your own key bindings)

# Controls

* Placing voxels via left mouse button
* Deleting voxels via right mouse button
* Rotating viewport via middle mouse button or holding alt and moving the mouse
* Holding the left or right mouse buttons spans an AABB to perform the action in
* Using the keybinding for `+actionexecute` (v) command allows you to also span an AABB with
  the cursor keys (resp. the keys that are bound to the `+movecursor[direction]` commands).
* Enter places the reference position at the current cursor position
* Shift+Tab opens the console (type cmdlist and cvarlist)

# Planned

* Rigging support
* Physics
* Particle emitter
* Copy/paste
* Selections
* Extrude
* Improve voxelizer
* Improve vox extension import
* Windows and MacOSX support
* Color selection via keyboard only
* Extend palette widget to show the used colors
* Add color dialog for ambient and diffuse color
* Export layers as single meshes
* Rotate around reference position
* Layer groups to e.g. rotate and move a whole group

# Needed improvements

* Allow to rename layers from the ui
* Fix memento states for merge, rename and move of layers

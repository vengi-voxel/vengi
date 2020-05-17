# Voxedit

## Purpose

Voxel editor.

This editor was written to have a linux voxel editor with animation support for
my own engine and evolved into something that others might find useful, too.

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/dwarf-in-editor.jpeg)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-custom-shader.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-quad-view.png)

## Features

* Load vox, qbt, qb, vxm, binvox, cub
* Save to vox, qbt, qb, cub
* Auto-saving
* Console to script your modeling (CTRL+TAB cmdlist)
* Key bindings via cfg file
* Four viewport mode
* Grid resolution
* Layer support
* Mirror mode
* Auto cropping volumes
* Auto generate content like trees or noise volumes
* Import heightmaps
* Undo/Redo
* Custom color palettes
* Reference positions to rotate around or create auto-generated structures at
* Viewport screenshots
* Modelling with the left and right mouse buttons
* Modelling without the mouse - just via shortcuts and keyboard
* Layer animation support
* Skeletal character animation support
* Custom key bindings
* Show commands of actions (to configure your own key bindings)
* Rotate around reference position or center of volume
* Import bitmaps as plane
* Copy/Cut/Paste - paste to cursor or reference position
* Exporting single layers into meshes

## Controls

* Placing voxels via left mouse button or SHIFT+[CTRL+]Cursors
* Deleting voxels via right mouse button
* Rotating viewport via middle mouse button or holding alt and moving the mouse
* Holding the left or right mouse buttons spans an AABB to perform the action in
* Using the keybinding for `+actionexecute` (v) command allows you to also span an AABB with
  the cursor keys (resp. the keys that are bound to the `+movecursor[direction]` commands).
* Enter places the reference position at the current cursor position
* SHIFT+Tab opens the console (type cmdlist and cvarlist)

## Planned

* SceneSettings dialog
  * Change sun angles
* Voxel to transparency with a HSV threshold
* Selections
  * Multiple AABBs should be possible
  * Deselect of single voxels
  * Delete, move, rotate, modify only in active selection
* Copy/paste
  * Paste into selection of a different size? Accept with enter after moving?
* Color selection via keyboard only
  * Shortcut for enter-color-in-the-next second and index of the colormap?
  * Relative movement from current color
* Improve Windows and MacOSX support
* Extend palette widget to show the used colors
* Add color dialog for ambient and diffuse color
* Extrude
* Improve vox extension import
* Rigging support
  * Gizmo for rotate, scale and move
  * Scene graph
  * File format (maybe vox with extension?)
  * UI for key frames (timeline)
* Physics
* Particle emitter

## Needed improvements

* Fix memento states for merge of layers
* Fix memento states for layer group modifications
* Fix memento states for translating a volume

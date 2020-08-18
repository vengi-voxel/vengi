# General

This is an opensource, cross platform voxel volume editor.

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/dwarf-in-editor.jpeg)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-custom-shader.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-quad-view.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/animation-frames.gif)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/quad-view-knight.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-command_and_conquer.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-duke.png)

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-import-planes.png)

## Features

* Auto-saving
* Console to script your modeling (CTRL+TAB cmdlist)
* LUA scripting api
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
* SHIFT+Tab opens the console (type `cmdlist` and `cvarlist`)

## Palette

The editor is built around a palette of 256 colors. Custom palettes are supported. The images should have a 1x256 pixel dimension.

In order to use some of the feature for procedurally generated content, you also have to define your material types with a lua script. See the existing palette lua script as a reference.

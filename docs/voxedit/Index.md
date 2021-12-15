# General

This is an opensource, cross platform voxel volume editor.

You can load and save a lot of different [voxel formats](../Formats.md).

![image](https://raw.githubusercontent.com/wiki/mgerhardy/engine/images/voxedit-new-ui.png)

## Features

* LUA [scripting](../LUAScript.md) api
* Layer support
* Auto cropping volumes
* Auto generate content like trees or noise volumes
* Import heightmaps
* Customizable UI
* L-System integration
* Undo/Redo
* Custom color palettes
* Viewport screenshots
* Layer animation support
* Custom key bindings
* Show commands of actions (to configure your own key bindings)
* Import bitmaps as plane
* Copy/Cut/Paste - paste to cursor or reference position
* Exporting single layers into meshes
* Generate level of details (LOD) volumes

## Controls

* Placing voxels via left mouse button or SHIFT+[CTRL+]Cursors
* Deleting voxels via right mouse button
* Rotating viewport via middle mouse button and moving the mouse
* Shift the camera by holding alt and moving the mouse
* Holding the left or right mouse buttons spans an AABB to perform the action in
* Using the keybinding for `+actionexecute` (v) command allows you to also span an AABB with
  the cursor keys (resp. the keys that are bound to the `+movecursor[direction]` commands).
* Enter places the reference position at the current cursor position
* SHIFT+Tab opens the console (type `cmdlist` and `cvarlist`)

## Palette

The editor is built around a palette of 256 colors. Custom palettes are supported. The images should have a 1x256 pixel dimension.

## Configuration

See `./vengi-voxedit --help` or [configuration](../Configuration.md) for more details.

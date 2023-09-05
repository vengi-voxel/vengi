# Usage

## Scene and edit mode

The scene mode is for animating and moving objects in the world - while the edit mode is for modifying voxels in there volumes.

You can toggle between those modes by pressing the `TAB` key in the default binding mode. See [controls](Controls.md).

## Reference position

The current reference position is rendered as a blue dot in the scene and is used to e.g. define the position of where to put procedurally generated content like the trees. It is also used to define the position of where to paste your clipboard at. But also some modifier are using the reference position.

The reference position can be set by pressing enter while hovering a particular voxel - or by using the command `setreferenceposition`.

# Brushes

## Modifiers

### Place

Place new voxels in your volume.

### Erase

Erase voxels from your volume. There is a shortcut when you are in the other modes to delete voxels: Press the right mouse button (at least in the default binding).

### Override

Override existing voxels with a new color - but also place new voxels.

### Paint

Other than **override** this modifier only changes the color of existing voxels.

### Path

This modifier puts voxels onto a path using the reference position as start and the position you clicked at as end. This needs solid voxels to work on. This doesn't work in empty volumes.

### Line

A line will just draw a line from the reference position to the position you clicked at.

### Color picker

You can either use it from the modifiers panel or by default with the key `p` to pick the color from the current selected voxel.

### Select

Span an selection box to operate in. Either for copy/pasting or to limit a certain action (like the [script](../LUAScript.md) execution). Don't forget to unselect (__Select__ -> __Select none__) before being able to operate on the whole volume again.

## Shape brush

You can use several shapes to create voxels via spanning a bounding box in the viewport volume by pressing and holding the left mouse button.

## Script brush

Execute [scripts](../LUAScript.md) with this brush.

## Plane brush

This allows you to extrude a whole plane.

## Stamp brush

Load stamps in form of other voxel assets via drag and drop or right click in the asset panel

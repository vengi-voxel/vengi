# Volume

## Methods

| Method | Description |
| ------ | ----------- |
| `clear()` | Clear all voxels in the volume (set to air). |
| `crop()` | Crop the volume to remove empty space around the voxels. |
| `erasePlane(x, y, z, face, groundColor, thickness)` | Erase connected voxels on a plane starting from a position. |
| `extrudePlane(x, y, z, face, groundColor, newColor, thickness)` | Extrude a plane of connected voxels from a position. |
| `fill(color, overwrite)` | Fill the entire volume with the specified color index. |
| `fillHollow(color)` | Fill hollow areas in the volume with the specified voxel color. |
| `fillPlane(image, searchVoxelColor, x, y, z, face)` | Fill a plane at the given position using colors from an image. |
| `hollow()` | Make the volume hollow by removing interior voxels. |
| `importColoredHeightmap(image, underground)` | Import a colored heightmap image into the volume. |
| `importHeightmap(image, underground, surface)` | Import a heightmap image into the volume. |
| `importImageAsVolume(texture, depthmap, palette, thickness, bothSides)` | Import an image as a 3D volume using depth information. |
| `isEmpty(minsx, minsy, minsz, maxsx, maxsy, maxsz)` | Check if a region is empty (contains only air). |
| `isTouching(x, y, z, connectivity)` | Check if a position is touching (adjacent to) a solid voxel. |
| `merge(source)` | Merge another volume into this one. |
| `mirrorAxis(axis)` | Mirror the volume along the specified axis. |
| `move(x, y, z)` | Move the voxels within the volume by the specified offset. |
| `normal(x, y, z)` | Get the normal palette index of the voxel at the specified coordinates. |
| `overridePlane(x, y, z, face, color, thickness)` | Override existing voxels on a plane with a new color. |
| `paintPlane(x, y, z, face, searchColor, replaceColor)` | Paint connected voxels on a plane with a new color. |
| `region()` | Get the region of the volume. |
| `remapToPalette(oldPalette, newPalette, skipColorIndex)` | Remap all voxel colors from an old palette to a new palette. |
| `renderIsometricImage(face)` | Render an isometric view of the volume to an image. |
| `renderToImage(face)` | Render the volume to a 2D image from the given face direction. |
| `resize(w, h, d, extendMins)` | Resize the volume by the specified amounts. |
| `rotateAxis(axis)` | Rotate the volume 90 degrees around the specified axis. |
| `rotateDegrees(angleX, angleY, angleZ, pivotX, pivotY, pivotZ)` | Rotate the volume by the given angles in degrees. |
| `scale(scaleX, scaleY, scaleZ, pivotX, pivotY, pivotZ)` | Scale the volume by the given scale factors. |
| `scaleDown()` | Scale the volume down by a factor of 2, averaging the colors. |
| `scaleUp()` | Scale the volume up by a factor of 2. |
| `setNormal(x, y, z, normal)` | Set the normal index on an existing voxel at the specified coordinates. |
| `setVoxel(x, y, z, color, normal)` | Set a voxel at the specified coordinates. |
| `text(font, text, x, y, z, size, thickness, spacing)` | Render text into the volume using a TrueType font. |
| `translate(x, y, z)` | Translate the region of the volume without moving the voxels. |
| `voxel(x, y, z)` | Get the voxel at the specified coordinates. |

## Detailed Documentation

### clear

Clear all voxels in the volume (set to air).

### crop

Crop the volume to remove empty space around the voxels.

### erasePlane

Erase connected voxels on a plane starting from a position.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `face` | `string` | The face direction (e.g. 'positiveX', 'negativeY', 'up', 'down', etc.). |
| `groundColor` | `integer` | The color index of the voxels to erase. |
| `thickness` | `integer` | The thickness of the erase (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels erased. |

### extrudePlane

Extrude a plane of connected voxels from a position.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `face` | `string` | The face direction (e.g. 'positiveX', 'negativeY', 'up', 'down', etc.). |
| `groundColor` | `integer` | The color index of the ground voxels to extrude. |
| `newColor` | `integer` | The color index for the new extruded voxels. |
| `thickness` | `integer` | The extrusion thickness (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels extruded. |

### fill

Fill the entire volume with the specified color index.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `color` | `integer` | The color index to fill with. |
| `overwrite` | `boolean` | If true, overwrite existing voxels. If false, only fill air voxels (optional, default true). |

### fillHollow

Fill hollow areas in the volume with the specified voxel color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `color` | `integer` | The color index to fill with (optional, default 1). |

### fillPlane

Fill a plane at the given position using colors from an image.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `image` | `image` | The image to use for filling colors. |
| `searchVoxelColor` | `integer` | The color index of the voxel to search for. |
| `x` | `integer` | The x coordinate to start at. |
| `y` | `integer` | The y coordinate to start at. |
| `z` | `integer` | The z coordinate to start at. |
| `face` | `string` | The face direction (e.g. 'positiveX', 'negativeY', 'up', 'down', etc.). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels filled. |

### hollow

Make the volume hollow by removing interior voxels.

### importColoredHeightmap

Import a colored heightmap image into the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `image` | `string` | Path to the colored heightmap image. |
| `underground` | `integer` | Color index for underground voxels (optional). |

### importHeightmap

Import a heightmap image into the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `image` | `string` | Path to the heightmap image. |
| `underground` | `integer` | Color index for underground voxels (optional). |
| `surface` | `integer` | Color index for surface voxels (optional). |

### importImageAsVolume

Import an image as a 3D volume using depth information.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `texture` | `string` | Path to the texture image. |
| `depthmap` | `string` | Path to the depth map image (optional). |
| `palette` | `palette` | Palette to use (optional). |
| `thickness` | `integer` | Thickness of the volume (optional, default 8). |
| `bothSides` | `boolean` | Create voxels on both sides (optional, default false). |

### isEmpty

Check if a region is empty (contains only air).

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `minsx` | `integer` | Minimum x coordinate (optional, defaults to volume region). |
| `minsy` | `integer` | Minimum y coordinate (optional). |
| `minsz` | `integer` | Minimum z coordinate (optional). |
| `maxsx` | `integer` | Maximum x coordinate (optional). |
| `maxsy` | `integer` | Maximum y coordinate (optional). |
| `maxsz` | `integer` | Maximum z coordinate (optional). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the region is empty. |

### isTouching

Check if a position is touching (adjacent to) a solid voxel.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `connectivity` | `string` | Connectivity type: '6' (faces), '18' (faces+edges), '26' (faces+edges+corners) (optional, default '6'). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the position is adjacent to a solid voxel. |

### merge

Merge another volume into this one.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `source` | `volume` | The source volume to merge from. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels merged. |

### mirrorAxis

Mirror the volume along the specified axis.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `axis` | `string` | The axis to mirror along: 'x', 'y', or 'z' (default 'y'). |

### move

Move the voxels within the volume by the specified offset.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x offset. |
| `y` | `integer` | The y offset (optional, default 0). |
| `z` | `integer` | The z offset (optional, default 0). |

### normal

Get the normal palette index of the voxel at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The normal palette index of the voxel (0 means no normal). |

### overridePlane

Override existing voxels on a plane with a new color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `face` | `string` | The face direction (e.g. 'positiveX', 'negativeY', 'up', 'down', etc.). |
| `color` | `integer` | The replacement color index. |
| `thickness` | `integer` | The override thickness (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels overridden. |

### paintPlane

Paint connected voxels on a plane with a new color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `face` | `string` | The face direction (e.g. 'positiveX', 'negativeY', 'up', 'down', etc.). |
| `searchColor` | `integer` | The color index to search for. |
| `replaceColor` | `integer` | The color index to replace with. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels painted. |

### region

Get the region of the volume.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `region` | The region of the volume. |

### remapToPalette

Remap all voxel colors from an old palette to a new palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `oldPalette` | `palette` | The old palette used by the current voxels. |
| `newPalette` | `palette` | The new palette to remap the colors to. |
| `skipColorIndex` | `integer` | An optional color index to skip during remapping (default: -1). |

### renderIsometricImage

Render an isometric view of the volume to an image.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `face` | `string` | The front face for the isometric view, e.g. 'front', 'back', 'left', 'right', 'up', 'down'. Optional, default: 'front'. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `image` | The rendered isometric image. |

### renderToImage

Render the volume to a 2D image from the given face direction.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `face` | `string` | The face to render from, e.g. 'front', 'back', 'left', 'right', 'up', 'down'. Optional, default: 'front'. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `image` | The rendered image. |

### resize

Resize the volume by the specified amounts.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `w` | `integer` | Width change. |
| `h` | `integer` | Height change (optional, default 0). |
| `d` | `integer` | Depth change (optional, default 0). |
| `extendMins` | `boolean` | Extend the minimum corner (optional, default false). |

### rotateAxis

Rotate the volume 90 degrees around the specified axis.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `axis` | `string` | The axis to rotate around: 'x', 'y', or 'z' (default 'y'). |

### rotateDegrees

Rotate the volume by the given angles in degrees.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angleX` | `integer` | The rotation angle around the x axis in degrees (must be a multiple of 90). |
| `angleY` | `integer` | The rotation angle around the y axis in degrees (must be a multiple of 90). (optional, default: 0) |
| `angleZ` | `integer` | The rotation angle around the z axis in degrees (must be a multiple of 90). (optional, default: 0) |
| `pivotX` | `number` | The normalized x pivot point (optional, default: 0.5). |
| `pivotY` | `number` | The normalized y pivot point (optional, default: 0.5). |
| `pivotZ` | `number` | The normalized z pivot point (optional, default: 0.5). |

### scale

Scale the volume by the given scale factors.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `scaleX` | `number` | The scale factor for the x axis. |
| `scaleY` | `number` | The scale factor for the y axis (optional, defaults to scaleX). |
| `scaleZ` | `number` | The scale factor for the z axis (optional, defaults to scaleX). |
| `pivotX` | `number` | The normalized x pivot point (optional, default: 0). |
| `pivotY` | `number` | The normalized y pivot point (optional, default: 0). |
| `pivotZ` | `number` | The normalized z pivot point (optional, default: 0). |

### scaleDown

Scale the volume down by a factor of 2, averaging the colors.

### scaleUp

Scale the volume up by a factor of 2.

### setNormal

Set the normal index on an existing voxel at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `normal` | `integer` | The normal palette index. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the voxel was updated, false if the voxel is air or outside the region. |

### setVoxel

Set a voxel at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `color` | `integer` | The color index to set, or -1 for air (optional, default 1). |
| `normal` | `integer` | The normal palette index (optional, default NO_NORMAL). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the voxel was set within the region, false otherwise. |

### text

Render text into the volume using a TrueType font.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `font` | `string` | Path to the TrueType font file. |
| `text` | `string` | The text to render. |
| `x` | `integer` | The x position (optional, default region lower x). |
| `y` | `integer` | The y position (optional, default region lower y). |
| `z` | `integer` | The z position (optional, default region lower z). |
| `size` | `integer` | Font size (optional, default 16). |
| `thickness` | `integer` | Voxel thickness (optional, default 1). |
| `spacing` | `integer` | Character spacing (optional, default 0). |

### translate

Translate the region of the volume without moving the voxels.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x translation. |
| `y` | `integer` | The y translation (optional, default 0). |
| `z` | `integer` | The z translation (optional, default 0). |

### voxel

Get the voxel at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The color index of the voxel at the specified coordinates, or -1 if the voxel is air. |


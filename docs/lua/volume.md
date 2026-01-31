# Volume

## Methods

| Method | Description |
| ------ | ----------- |
| `crop()` | Crop the volume to remove empty space around the voxels. |
| `fillHollow(color)` | Fill hollow areas in the volume with the specified voxel color. |
| `hollow()` | Make the volume hollow by removing interior voxels. |
| `importColoredHeightmap(image, underground)` | Import a colored heightmap image into the volume. |
| `importHeightmap(image, underground, surface)` | Import a heightmap image into the volume. |
| `importImageAsVolume(texture, depthmap, palette, thickness, bothSides)` | Import an image as a 3D volume using depth information. |
| `mirrorAxis(axis)` | Mirror the volume along the specified axis. |
| `move(x, y, z)` | Move the voxels within the volume by the specified offset. |
| `region()` | Get the region of the volume. |
| `resize(w, h, d, extendMins)` | Resize the volume by the specified amounts. |
| `rotateAxis(axis)` | Rotate the volume 90 degrees around the specified axis. |
| `setVoxel(x, y, z, color)` | Set a voxel at the specified coordinates. |
| `text(font, text, x, y, z, size, thickness, spacing)` | Render text into the volume using a TrueType font. |
| `translate(x, y, z)` | Translate the region of the volume without moving the voxels. |
| `voxel(x, y, z)` | Get the voxel at the specified coordinates. |

## Detailed Documentation

### crop

Crop the volume to remove empty space around the voxels.

### fillHollow

Fill hollow areas in the volume with the specified voxel color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `color` | `integer` | The color index to fill with (optional, default 1). |

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

### region

Get the region of the volume.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `region` | The region of the volume. |

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

### setVoxel

Set a voxel at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `color` | `integer` | The color index to set, or -1 for air (optional, default 1). |

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


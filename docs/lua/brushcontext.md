# BrushContext

Global: `g_brushcontext`

## Functions

| Function | Description |
| -------- | ----------- |
| `brushGizmoActive()` | Check whether the brush gizmo is actively being manipulated. |
| `color()` | Get the current palette color index. |
| `cursorFace()` | Get the face direction where the raycast hit. |
| `cursorPos()` | Get the current cursor position. |
| `fixedOrthoSideView()` | Check whether the view is an orthographic side view. |
| `gridResolution()` | Get the grid resolution for snapping. |
| `hitCursorColor()` | Get the palette color index of the voxel hit by raycast (before the hit face). |
| `lockedAxis()` | Get the axis lock constraint for 2D operations. |
| `modifierType()` | Get the current modifier operation type. |
| `normalIndex()` | Get the normal index used for normal painting. |
| `normalPaint()` | Check whether normal painting mode is active. |
| `prevCursorPos()` | Get the cursor position before any clamping or brush execution was applied. |
| `referencePos()` | Get the reference position set by the user before applying the brush. |
| `targetVolumeRegion()` | Get the region of the target volume for clamping the brush. |
| `voxelAtCursorColor()` | Get the palette color index of the voxel at the cursor position (may be air = -1). |

## Detailed Documentation

### brushGizmoActive

Check whether the brush gizmo is actively being manipulated.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the gizmo is being manipulated. |

### color

Get the current palette color index.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The palette color index. |

### cursorFace

Get the face direction where the raycast hit.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | One of 'left', 'right', 'up', 'down', 'front', 'back', or 'max'. |

### cursorPos

Get the current cursor position.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The cursor position as a table with x, y, z fields. |

### fixedOrthoSideView

Check whether the view is an orthographic side view.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if in orthographic side view mode. |

### gridResolution

Get the grid resolution for snapping.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Grid resolution - voxels are placed at multiples of this value. |

### hitCursorColor

Get the palette color index of the voxel hit by raycast (before the hit face).

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The palette color index of the hit voxel. |

### lockedAxis

Get the axis lock constraint for 2D operations.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | One of 'none', 'x', 'y', or 'z'. |

### modifierType

Get the current modifier operation type.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | One of 'place', 'erase', 'override', 'paint', 'normalpaint', or 'colorpicker'. |

### normalIndex

Get the normal index used for normal painting.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The normal index value. |

### normalPaint

Check whether normal painting mode is active.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if normal painting mode is active. |

### prevCursorPos

Get the cursor position before any clamping or brush execution was applied.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The previous cursor position as a table with x, y, z fields. |

### referencePos

Get the reference position set by the user before applying the brush.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The reference position as a table with x, y, z fields. |

### targetVolumeRegion

Get the region of the target volume for clamping the brush.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `Region` | The region of the target volume. |

### voxelAtCursorColor

Get the palette color index of the voxel at the cursor position (may be air = -1).

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The palette color index of the voxel at cursor, or -1 for air. |


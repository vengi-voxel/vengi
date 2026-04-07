# SelectionContext

Global: `g_selectioncontext`

## Functions

| Function | Description |
| -------- | ----------- |
| `aabbFace()` | Get the face from the initial click. |
| `aabbFirstPos()` | Get the position of the initial click that started the selection. |
| `cursorFace()` | Get the face direction where the raycast hit. |
| `cursorPos()` | Get the current cursor position. |
| `hitCursorColor()` | Get the palette color index of the voxel hit by raycast. |
| `modifierType()` | Get the current modifier type (select or deselect). |
| `referencePos()` | Get the reference position set by the user. |
| `targetVolumeRegion()` | Get the region of the target volume. |

## Detailed Documentation

### aabbFace

Get the face from the initial click.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | One of 'left', 'right', 'up', 'down', 'front', 'back', or 'max'. |

### aabbFirstPos

Get the position of the initial click that started the selection.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The first click position. |

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
| `ivec3` | The cursor position. |

### hitCursorColor

Get the palette color index of the voxel hit by raycast.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The palette color index of the hit voxel. |

### modifierType

Get the current modifier type (select or deselect).

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | One of 'override' (select) or 'erase' (deselect). |

### referencePos

Get the reference position set by the user.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The reference position. |

### targetVolumeRegion

Get the region of the target volume.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `Region` | The region of the target volume. |


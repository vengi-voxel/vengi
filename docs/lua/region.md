# Region

Global: `g_region`

## Methods

| Method | Description |
| ------ | ----------- |
| `center()` | Get the center point of the region. |
| `contains(other)` | Check if this region fully contains another region. |
| `depth()` | Get the depth of the region in voxels. |
| `height()` | Get the height of the region in voxels. |
| `intersects(other)` | Check if this region intersects with another region. |
| `isOnBorder(pos)` | Check if a position is on the border of the region. |
| `maxs()` | Get the upper corner of the region. |
| `mins()` | Get the lower corner of the region. |
| `new(minX, minY, minZ, maxX, maxY, maxZ)` | Create a new region with the specified bounds. |
| `setMaxs(maxs)` | Set the upper corner of the region. |
| `setMins(mins)` | Set the lower corner of the region. |
| `size()` | Get the dimensions of the region. |
| `width()` | Get the width of the region in voxels. |
| `x()` | Get the lower x coordinate of the region. |
| `y()` | Get the lower y coordinate of the region. |
| `z()` | Get the lower z coordinate of the region. |

## Detailed Documentation

### center

Get the center point of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `vec3` | The center point of the region. |

### contains

Check if this region fully contains another region.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `other` | `region` | The other region to check. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if this region contains the other, false otherwise. |

### depth

Get the depth of the region in voxels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The depth of the region. |

### height

Get the height of the region in voxels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The height of the region. |

### intersects

Check if this region intersects with another region.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `other` | `region` | The other region to check. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if regions intersect, false otherwise. |

### isOnBorder

Check if a position is on the border of the region.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `ivec3` | The position to check. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if on border, false otherwise. |

### maxs

Get the upper corner of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The upper corner coordinates. |

### mins

Get the lower corner of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The lower corner coordinates. |

### new

Create a new region with the specified bounds.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `minX` | `integer` | Minimum x coordinate. |
| `minY` | `integer` | Minimum y coordinate. |
| `minZ` | `integer` | Minimum z coordinate. |
| `maxX` | `integer` | Maximum x coordinate. |
| `maxY` | `integer` | Maximum y coordinate. |
| `maxZ` | `integer` | Maximum z coordinate. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `region` | The newly created region. |

### setMaxs

Set the upper corner of the region.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `maxs` | `ivec3` | The new upper corner coordinates. |

### setMins

Set the lower corner of the region.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `mins` | `ivec3` | The new lower corner coordinates. |

### size

Get the dimensions of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `ivec3` | The dimensions in voxels. |

### width

Get the width of the region in voxels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The width of the region. |

### x

Get the lower x coordinate of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The lower x coordinate. |

### y

Get the lower y coordinate of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The lower y coordinate. |

### z

Get the lower z coordinate of the region.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The lower z coordinate. |


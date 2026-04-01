# SparseVolume

Global: `g_sparsevolume`

## Methods

| Method | Description |
| ------ | ----------- |
| `clear()` | Remove all voxels from the sparse volume. |
| `copyFromVolume(volume)` | Copy all solid voxels from a volume into the sparse volume. |
| `copyToVolume(volume)` | Copy all voxels from the sparse volume into a volume. |
| `empty()` | Check if the sparse volume has no voxels. |
| `new(minsx, minsy, minsz, maxsx, maxsy, maxsz)` | Create a new sparse volume. Optionally specify a limiting region. |
| `region()` | Get the region of the sparse volume. If no limiting region was set, calculates bounds from stored voxels. |
| `setVoxel(x, y, z, color)` | Set the voxel at the specified coordinates. |
| `size()` | Get the number of stored voxels. |
| `voxel(x, y, z)` | Get the voxel color index at the specified coordinates. |

## Detailed Documentation

### clear

Remove all voxels from the sparse volume.

### copyFromVolume

Copy all solid voxels from a volume into the sparse volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The source volume to copy from. |

### copyToVolume

Copy all voxels from the sparse volume into a volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The target volume to copy into. |

### empty

Check if the sparse volume has no voxels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the volume is empty. |

### new

Create a new sparse volume. Optionally specify a limiting region.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `minsx` | `integer` | The min x coordinate (optional). |
| `minsy` | `integer` | The min y coordinate (optional). |
| `minsz` | `integer` | The min z coordinate (optional). |
| `maxsx` | `integer` | The max x coordinate (optional). |
| `maxsy` | `integer` | The max y coordinate (optional). |
| `maxsz` | `integer` | The max z coordinate (optional). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `sparsevolume` | A new sparse volume instance. |

### region

Get the region of the sparse volume. If no limiting region was set, calculates bounds from stored voxels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `region` | The region of the volume. |

### setVoxel

Set the voxel at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |
| `color` | `integer` | The color index (optional, default 1). Use -1 to set air. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the voxel was set. |

### size

Get the number of stored voxels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of voxels stored. |

### voxel

Get the voxel color index at the specified coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `integer` | The x coordinate. |
| `y` | `integer` | The y coordinate. |
| `z` | `integer` | The z coordinate. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The color index of the voxel, or -1 if air. |


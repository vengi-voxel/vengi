# sculpt

Global: `g_sculpt`

## Functions

| Function | Description |
| -------- | ----------- |
| `bridgegap(volume, color)` | Connect boundary voxels by drawing 3D lines between all pairs, filling air along each line. Bridges gaps and cracks across openings. |
| `erode(volume, strength, iterations)` | Erode surface voxels based on their solid face-neighbor count. Voxels with fewer neighbors are more exposed and get removed first. |
| `flatten(volume, face, iterations)` | Flatten by peeling layers from the outermost surface along a face normal direction. |
| `grow(volume, strength, iterations, color)` | Grow into air positions adjacent to the surface. Air with more solid neighbors fills first. |
| `reskin(volume, skin, face, mode, follow, skinDepth, surfaceOffset, skinUpAxis)` | Apply a skin volume (texture pattern) onto the selected surface. The skin tiles across the selection and can replace, blend, or carve voxels. |
| `smoothadditive(volume, face, heightThreshold, iterations, color)` | Fill height gaps by scanning layers along a face normal. Air voxels on solid ground get filled when a neighbor column is significantly taller. Each iteration adds at most one voxel per column. |
| `smootherode(volume, face, iterations, preserveTopHeight, trimPerStep)` | Remove edge voxels from the top of columns along a face normal. Scans top-to-bottom, removing top-of-column voxels that have fewer than 4 solid planar neighbors. Each iteration removes at most one voxel per column. |
| `smoothgaussian(volume, face, kernelSize, sigma, iterations, color)` | Blur the height map using a 2D Gaussian kernel along a face normal. Columns taller than the weighted average are trimmed, shorter ones are filled. Uses circular sampling within the kernel radius. |
| `smoothwall(volume, face, iterations, color, removeAboveDepth, interpolation)` | Smooth a wall surface by interpolating interior column heights from nearest edge columns in 4 directions. |
| `squashtoplane(volume, face, planeCoord)` | Project all solid voxels onto a single plane. For each column along the face normal, if any voxel exists, one is placed at the plane coordinate. All others are removed. |

## Detailed Documentation

### bridgegap

Connect boundary voxels by drawing 3D lines between all pairs, filling air along each line. Bridges gaps and cracks across openings.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to bridge. |
| `color` | `integer` | Palette color index for new voxels (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### erode

Erode surface voxels based on their solid face-neighbor count. Voxels with fewer neighbors are more exposed and get removed first.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to erode. |
| `strength` | `number` | Erosion strength in [0, 1] (optional, default 0.5). |
| `iterations` | `integer` | Number of erosion passes (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### flatten

Flatten by peeling layers from the outermost surface along a face normal direction.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to flatten. |
| `face` | `string` | Face direction: 'up', 'down', 'left', 'right', 'front', 'back'. |
| `iterations` | `integer` | Number of layers to peel (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### grow

Grow into air positions adjacent to the surface. Air with more solid neighbors fills first.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to grow. |
| `strength` | `number` | Growth strength in [0, 1] (optional, default 0.5). |
| `iterations` | `integer` | Number of growth passes (optional, default 1). |
| `color` | `integer` | Palette color index for new voxels (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### reskin

Apply a skin volume (texture pattern) onto the selected surface. The skin tiles across the selection and can replace, blend, or carve voxels.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The target volume to reskin. |
| `skin` | `volume` | The skin volume providing the pattern. |
| `face` | `string` | Face direction defining surface normal: 'up', 'down', 'left', 'right', 'front', 'back'. |
| `mode` | `string` | Reskin mode: 'replace' (skin overwrites, air removes), 'blend' (skin overwrites, air preserves), 'negate' (skin removes, air preserves) (optional, default 'blend'). |
| `follow` | `string` | Surface follow mode: 'none' (flat plane), 'median' (median height plane), 'voxel' (per-column) (optional, default 'voxel'). |
| `skinDepth` | `integer` | Number of skin layers to apply (optional, default: full skin depth along up axis). |
| `surfaceOffset` | `integer` | Offset from surface: positive = above, negative = below (optional, default 0). |
| `skinUpAxis` | `string` | Which skin axis is outward: 'x', 'y', 'z' (optional, default 'y'). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### smoothadditive

Fill height gaps by scanning layers along a face normal. Air voxels on solid ground get filled when a neighbor column is significantly taller. Each iteration adds at most one voxel per column.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to smooth. |
| `face` | `string` | Face direction defining 'up': 'up', 'down', 'left', 'right', 'front', 'back'. |
| `heightThreshold` | `integer` | Minimum height difference in voxels for filling (optional, default 1). 1 = aggressive, higher = conservative. |
| `iterations` | `integer` | Number of smoothing passes (optional, default 1). Each pass adds at most one voxel per column. |
| `color` | `integer` | Palette color index for new voxels (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### smootherode

Remove edge voxels from the top of columns along a face normal. Scans top-to-bottom, removing top-of-column voxels that have fewer than 4 solid planar neighbors. Each iteration removes at most one voxel per column.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to erode. |
| `face` | `string` | Face direction defining 'up': 'up', 'down', 'left', 'right', 'front', 'back'. |
| `iterations` | `integer` | Number of erosion passes (optional, default 1). Each pass removes at most one voxel per column. |
| `preserveTopHeight` | `boolean` | If true, use island-based slope trimming to create pyramid shapes instead of uniform erosion (optional, default false). |
| `trimPerStep` | `integer` | When preserveTopHeight is true, number of voxels trimmed per unit distance from island center (optional, default 1). Higher values create steeper slopes. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### smoothgaussian

Blur the height map using a 2D Gaussian kernel along a face normal. Columns taller than the weighted average are trimmed, shorter ones are filled. Uses circular sampling within the kernel radius.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to smooth. |
| `face` | `string` | Face direction defining 'up': 'up', 'down', 'left', 'right', 'front', 'back'. |
| `kernelSize` | `integer` | Radius of the Gaussian kernel (optional, default 1). 1=3x3, 2=5x5, 3=7x7, 4=9x9. |
| `sigma` | `number` | Standard deviation of the Gaussian bell curve (optional, default 1.0). Lower = sharper, higher = broader smoothing. |
| `iterations` | `integer` | Number of blur passes (optional, default 1). |
| `color` | `integer` | Palette color index for new voxels (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### smoothwall

Smooth a wall surface by interpolating interior column heights from nearest edge columns in 4 directions. Edge columns (selected voxels bordering non-selected) are preserved for seamless blending. A gap-fill pass extends columns downward to match the lowest neighbor, preventing holes at corners.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to smooth. |
| `face` | `string` | Face direction defining the surface normal: 'up', 'down', 'left', 'right', 'front', 'back'. |
| `iterations` | `integer` | Number of smoothing passes (optional, default 1). |
| `color` | `integer` | Palette color index for new voxels (optional, default 1). |
| `removeAboveDepth` | `integer` | How many voxels above the smooth surface to clear (optional, default 0 = don't clear). |
| `interpolation` | `string` | Interpolation mode: 'linear', 'inversedistance' (default), or 'edgeaware'. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |

### squashtoplane

Project all solid voxels onto a single plane. For each column along the face normal, if any voxel exists, one is placed at the plane coordinate. All others are removed.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to squash. |
| `face` | `string` | Face direction defining the column axis: 'up', 'down', 'left', 'right', 'front', 'back'. |
| `planeCoord` | `integer` | The coordinate along the face axis where voxels are projected to. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Number of voxels changed. |


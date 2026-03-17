# sculpt

Global: `g_sculpt`

## Functions

| Function | Description |
| -------- | ----------- |
| `bridgegap(volume, color)` | Connect boundary voxels by drawing 3D lines between all pairs, filling air along each line. Bridges gaps and cracks across openings. |
| `erode(volume, strength, iterations)` | Erode surface voxels based on their solid face-neighbor count. Voxels with fewer neighbors are more exposed and get removed first. |
| `flatten(volume, face, iterations)` | Flatten by peeling layers from the outermost surface along a face normal direction. |
| `grow(volume, strength, iterations, color)` | Grow into air positions adjacent to the surface. Air with more solid neighbors fills first. |
| `smoothadditive(volume, face, heightThreshold, iterations, color)` | Fill height gaps by scanning layers along a face normal. Air voxels on solid ground get filled when a neighbor column is significantly taller. Each iteration adds at most one voxel per column. |
| `smootherode(volume, face, iterations, preserveTopHeight, trimPerStep)` | Remove edge voxels from the top of columns along a face normal. Scans top-to-bottom, removing top-of-column voxels that have fewer than 4 solid planar neighbors. Each iteration removes at most one voxel per column. |
| `smoothgaussian(volume, face, kernelSize, sigma, iterations, color)` | Blur the height map using a 2D Gaussian kernel along a face normal. Columns taller than the weighted average are trimmed, shorter ones are filled. Uses circular sampling within the kernel radius. |

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


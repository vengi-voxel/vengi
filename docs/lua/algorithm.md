# Algorithm

Global: `g_algorithm`

## Functions

| Function | Description |
| -------- | ----------- |
| `genland(seed, size, height, octaves, smoothing, persistence, amplitude, riverWidth, freqGround, freqRiver, offsetX, offsetZ, shadow, river, ambience)` | Generate procedural terrain. |
| `shadow(volume, lightStep)` | Add shadow coloring to a volume. |

## Detailed Documentation

### genland

Generate procedural terrain.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `seed` | `integer` | Random seed (optional, default 0). |
| `size` | `integer` | Terrain size (optional, default 256). |
| `height` | `integer` | Max height (optional, default 64). |
| `octaves` | `integer` | Noise octaves (optional, default 10). |
| `smoothing` | `number` | Smoothing factor (optional, default 1). |
| `persistence` | `number` | Noise persistence (optional, default 0.4). |
| `amplitude` | `number` | Noise amplitude (optional, default 0.4). |
| `riverWidth` | `number` | River width (optional, default 0.02). |
| `freqGround` | `number` | Ground frequency (optional, default 9.5). |
| `freqRiver` | `number` | River frequency (optional, default 13.2). |
| `offsetX` | `integer` | X offset (optional, default 0). |
| `offsetZ` | `integer` | Z offset (optional, default 0). |
| `shadow` | `boolean` | Add shadows (optional, default true). |
| `river` | `boolean` | Add rivers (optional, default true). |
| `ambience` | `boolean` | Add ambient effects (optional, default true). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The generated terrain node. |

### shadow

Add shadow coloring to a volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to add shadows to. |
| `lightStep` | `integer` | Light step value (optional, default 8). |


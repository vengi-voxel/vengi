# Algorithm

Global: `g_algorithm`

## Functions

| Function | Description |
| -------- | ----------- |
| `genland(seed, size, height, octaves, smoothing, persistence, amplitude, baseHeight, riverWidth, numRivers, riverPhase, riverMeander, freqGround, freqRiver, grassBias, offsetX, offsetZ, shadow, shadowFactor, river, ambience, ambienceFactor, groundColor, grassColor, grass2Color, waterColor)` | Generate procedural terrain. |
| `shadow(volume, lightStep)` | Add shadow coloring to a volume. |

## Detailed Documentation

### genland

Generate procedural terrain.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `seed` | `integer` | Random seed (optional, default 0). |
| `size` | `integer` | Terrain size (optional, default 256). |
| `height` | `integer` | Volume height clip (optional, default 64). |
| `octaves` | `integer` | Noise octaves (optional, default 10). |
| `smoothing` | `number` | Shadow smoothing iterations (optional, default 1). |
| `persistence` | `number` | Octave amplitude falloff (optional, default 0.4). |
| `amplitude` | `number` | Height noise strength from baseHeight (optional, default 20). |
| `baseHeight` | `number` | Average column height before noise (optional, default 28). |
| `riverWidth` | `number` | River width (optional, default 0.02). |
| `numRivers` | `integer` | Number of rivers across the map (optional, default 1). |
| `riverPhase` | `number` | Horizontal river start position 0-1 (optional, default 0.75). |
| `riverMeander` | `number` | River path winding strength (optional, default 4). |
| `freqGround` | `number` | Ground frequency (optional, default 9.5). |
| `freqRiver` | `number` | River frequency (optional, default 13.2). |
| `grassBias` | `number` | Grass tint bias, positive = more grass (optional, default 0). |
| `offsetX` | `integer` | X offset (optional, default 0). |
| `offsetZ` | `integer` | Z offset (optional, default 0). |
| `shadow` | `boolean` | Add shadows (optional, default true). |
| `shadowFactor` | `integer` | Shadow strength 0-255 (optional, default 32). |
| `river` | `boolean` | Add rivers (optional, default true). |
| `ambience` | `boolean` | Add ambient lighting (optional, default true). |
| `ambienceFactor` | `number` | Ambient color scale (optional, default 0.3). |
| `groundColor` | `integer` | Ground color palette index (optional). |
| `grassColor` | `integer` | Grass color palette index (optional). |
| `grass2Color` | `integer` | Secondary grass color palette index (optional). |
| `waterColor` | `integer` | Water color palette index (optional). |

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


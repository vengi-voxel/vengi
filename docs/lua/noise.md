# Noise

Global: `g_noise`

## Functions

| Function | Description |
| -------- | ----------- |
| `fBm2(pos, octaves, lacunarity, gain)` | Generate 2D fractal Brownian motion noise. |
| `fBm3(pos, octaves, lacunarity, gain)` | Generate 3D fractal Brownian motion noise. |
| `fBm4(pos, octaves, lacunarity, gain)` | Generate 4D fractal Brownian motion noise. |
| `noise2(x, y)` | Generate 2D simplex noise. |
| `noise3(x, y, z)` | Generate 3D simplex noise. |
| `noise4(x, y, z, w)` | Generate 4D simplex noise. |
| `ridgedMF2(pos, ridgeOffset, octaves, lacunarity, gain)` | Generate 2D ridged multi-fractal noise. |
| `ridgedMF3(pos, ridgeOffset, octaves, lacunarity, gain)` | Generate 3D ridged multi-fractal noise. |
| `ridgedMF4(pos, ridgeOffset, octaves, lacunarity, gain)` | Generate 4D ridged multi-fractal noise. |
| `swissTurbulence(pos, offset, octaves, lacunarity, gain, warp)` | Generate Swiss turbulence noise. |
| `voronoi(pos, frequency, seed, enableDistance)` | Generate Voronoi noise. |
| `worley2(pos)` | Generate 2D Worley (cellular) noise. |
| `worley3(pos)` | Generate 3D Worley (cellular) noise. |

## Detailed Documentation

### fBm2

Generate 2D fractal Brownian motion noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec2` | The 2D position. |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.5). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | fBm noise value. |

### fBm3

Generate 3D fractal Brownian motion noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec3` | The 3D position. |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.5). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | fBm noise value. |

### fBm4

Generate 4D fractal Brownian motion noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec4` | The 4D position. |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.5). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | fBm noise value. |

### noise2

Generate 2D simplex noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | The x coordinate (or vec2). |
| `y` | `number` | The y coordinate (optional if vec2 provided). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Noise value in range [-1, 1]. |

### noise3

Generate 3D simplex noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | The x coordinate (or vec3). |
| `y` | `number` | The y coordinate (optional if vec3 provided). |
| `z` | `number` | The z coordinate (optional if vec3 provided). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Noise value in range [-1, 1]. |

### noise4

Generate 4D simplex noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | The x coordinate (or vec4). |
| `y` | `number` | The y coordinate (optional if vec4 provided). |
| `z` | `number` | The z coordinate (optional if vec4 provided). |
| `w` | `number` | The w coordinate (optional if vec4 provided). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Noise value in range [-1, 1]. |

### ridgedMF2

Generate 2D ridged multi-fractal noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec2` | The 2D position. |
| `ridgeOffset` | `number` | Ridge offset (optional, default 1.0). |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.5). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Ridged multi-fractal noise value. |

### ridgedMF3

Generate 3D ridged multi-fractal noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec3` | The 3D position. |
| `ridgeOffset` | `number` | Ridge offset (optional, default 1.0). |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.5). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Ridged multi-fractal noise value. |

### ridgedMF4

Generate 4D ridged multi-fractal noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec4` | The 4D position. |
| `ridgeOffset` | `number` | Ridge offset (optional, default 1.0). |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.5). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Ridged multi-fractal noise value. |

### swissTurbulence

Generate Swiss turbulence noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec2` | The 2D position. |
| `offset` | `number` | Offset (optional, default 1.0). |
| `octaves` | `integer` | Number of octaves (optional, default 4). |
| `lacunarity` | `number` | Lacunarity (optional, default 2.0). |
| `gain` | `number` | Gain (optional, default 0.6). |
| `warp` | `number` | Warp amount (optional, default 0.15). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Swiss turbulence noise value. |

### voronoi

Generate Voronoi noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec3` | The 3D position. |
| `frequency` | `number` | Frequency (optional, default 1.0). |
| `seed` | `integer` | Random seed (optional, default 0). |
| `enableDistance` | `boolean` | Enable distance output (optional, default true). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Voronoi noise value. |

### worley2

Generate 2D Worley (cellular) noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec2` | The 2D position. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Worley noise value. |

### worley3

Generate 3D Worley (cellular) noise.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pos` | `vec3` | The 3D position. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | Worley noise value. |


# lsystem

Global: `g_lsystem`

## Functions

| Function | Description |
| -------- | ----------- |
| `commands()` | Get the list of available L-system turtle commands and their descriptions. |
| `generate(volume, axiom, rules, colorIndex, angle, length, width, widthIncrement, iterations, leafRadius, posX, posY, posZ, options)` | Generate voxels using an L-system (Lindenmayer system). The L-system is executed to completion and the result is placed into the given volume. |
| `templates()` | Get all built-in L-system templates. Each template contains name, description, axiom, rules, angle, length, width, widthIncrement, iterations, leafRadius, lengthScale, minLength, angleVariation (degrees), seed, and leafColor. |

## Detailed Documentation

### commands

Get the list of available L-system turtle commands and their descriptions.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | An array of command tables with fields: command (single character), description. |

### generate

Generate voxels using an L-system (Lindenmayer system). The L-system is executed to completion and the result is placed into the given volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to place the L-system result into. |
| `axiom` | `string` | The initial axiom string (e.g. 'X' or 'F'). |
| `rules` | `string` | The production rules in format '{ A rule } { B rule }' (e.g. '{ X F[+X][-X] } { F FF }'). |
| `colorIndex` | `integer` | The initial palette color index (optional, default 0). |
| `angle` | `number` | The rotation angle in degrees (optional, default 25). |
| `length` | `number` | The line length per step (optional, default 1). |
| `width` | `number` | The initial line width (optional, default 1). |
| `widthIncrement` | `number` | Width change per '#' or '!' command (optional, default 0.5). |
| `iterations` | `integer` | Number of rule expansion iterations (optional, default 4). |
| `leafRadius` | `number` | Radius for leaf spheres (optional, default 8). |
| `posX` | `integer` | X position offset (optional, default 0). |
| `posY` | `integer` | Y position offset (optional, default 0). |
| `posZ` | `integer` | Z position offset (optional, default 0). |
| `options` | `table` | Optional settings: lengthScale (default 1), minLength (default 0.5), angleVariation in degrees (default 0), seed (default 1), leafColor palette index (default -1 uses current voxel). A template table can be passed directly. |

### templates

Get all built-in L-system templates. Each template contains name, description, axiom, rules, angle, length, width, widthIncrement, iterations, leafRadius, lengthScale, minLength, angleVariation (degrees), seed, and leafColor.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | An array of template tables with fields: name, description, axiom, rules, angle, length, width, widthIncrement, iterations, leafRadius, lengthScale, minLength, angleVariation (degrees), seed, leafColor. |


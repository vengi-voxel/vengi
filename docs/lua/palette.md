# Palette

Global: `g_palette`

## Methods

| Method | Description |
| ------ | ----------- |
| `color(index)` | Get a color from the palette as vec4. |
| `colorString(index)` | Get a string representation of a palette color. |
| `colors()` | Get all colors in the palette as a table of vec4. |
| `deltaE(index1, index2)` | Calculate the perceptual color difference (Delta E 76) between two palette colors. |
| `load(name)` | Load a palette from a file or built-in name. |
| `match(r, g, b, skipIndex)` | Find the closest matching color in the palette. |
| `material(index, property)` | Get a material property for a palette color. |
| `new()` | Create a new empty palette. |
| `rgba(index)` | Get a color from the palette as separate RGBA components. |
| `setColor(index, r, g, b, a)` | Set a color in the palette. |
| `setMaterial(index, property, value)` | Set a material property for a palette color. |
| `similar(index, count)` | Find similar colors in the palette. |
| `size()` | Get the number of colors in the palette. |

## Detailed Documentation

### color

Get a color from the palette as vec4.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `vec4` | The color as RGBA vec4 (0-1 range). |

### colorString

Get a string representation of a palette color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | String representation of the color. |

### colors

Get all colors in the palette as a table of vec4.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | Table of vec4 colors (RGBA, 0-1 range). |

### deltaE

Calculate the perceptual color difference (Delta E 76) between two palette colors.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index1` | `integer` | First color index. |
| `index2` | `integer` | Second color index. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The Delta E value (0 = identical colors). |

### load

Load a palette from a file or built-in name.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | File path or built-in palette name (e.g., 'built-in:minecraft'). |

### match

Find the closest matching color in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `r` | `integer` | Red component (0-255). |
| `g` | `integer` | Green component (0-255). |
| `b` | `integer` | Blue component (0-255). |
| `skipIndex` | `integer` | Index to skip (optional, default -1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The index of the closest matching color. |

### material

Get a material property for a palette color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |
| `property` | `string` | The property name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The property value. |

### new

Create a new empty palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `palette` | The newly created palette. |

### rgba

Get a color from the palette as separate RGBA components.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | Red component (0-255). |
| `integer` | Green component (0-255). |
| `integer` | Blue component (0-255). |
| `integer` | Alpha component (0-255). |

### setColor

Set a color in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |
| `r` | `integer` | Red component (0-255). |
| `g` | `integer` | Green component (0-255). |
| `b` | `integer` | Blue component (0-255). |
| `a` | `integer` | Alpha component (0-255, optional, default 255). |

### setMaterial

Set a material property for a palette color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |
| `property` | `string` | The property name. |
| `value` | `number` | The property value. |

### similar

Find similar colors in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The reference color index. |
| `count` | `integer` | Number of similar colors to find. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | Table of similar color indices, or nil if none found. |

### size

Get the number of colors in the palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of colors. |


# Palette

Global: `g_palette`

## Methods

| Method | Description |
| ------ | ----------- |
| `brighter(factor)` | Make the palette colors brighter. |
| `changeIntensity(scale)` | Change the color intensity of the palette. |
| `colder(value)` | Make the palette colors colder. |
| `color(index)` | Get a color from the palette as vec4. |
| `colorName(index)` | Get the name of a color in the palette. |
| `colorString(index)` | Get a string representation of a palette color. |
| `colors()` | Get all colors in the palette as a table of vec4. |
| `contrastStretching()` | Apply contrast stretching to the palette. |
| `copy(from, to)` | Copy a color from one slot to another. |
| `darker(factor)` | Make the palette colors darker. |
| `deltaE(index1, index2)` | Calculate the perceptual color difference (Delta E 76) between two palette colors. |
| `duplicateColor(index)` | Duplicate a color to a new slot in the palette. |
| `exchange(index1, index2)` | Exchange (swap) two colors in the palette. |
| `fill()` | Fill the remaining palette slots with black. |
| `hasAlpha(index)` | Check if a palette color has alpha transparency. |
| `hasColor(r, g, b)` | Check if a color exists in the palette. |
| `hasEmit(index)` | Check if a palette color is emissive. |
| `hasFreeSlot()` | Check if the palette has a free slot for a new color. |
| `hasMaterials()` | Check if the palette has any materials set. |
| `hash()` | Get the hash of the palette. |
| `load(name)` | Load a palette from a file or built-in name. |
| `match(r, g, b, skipIndex)` | Find the closest matching color in the palette. |
| `material(index, property)` | Get a material property for a palette color. |
| `name()` | Get the name of the palette. |
| `new()` | Create a new empty palette. |
| `reduce(targetColors)` | Reduce the palette to a target number of colors. |
| `removeColor(index)` | Remove a color from the palette. |
| `rgba(index)` | Get a color from the palette as separate RGBA components. |
| `save(name)` | Save the palette to a file. |
| `setColor(index, r, g, b, a)` | Set a color in the palette. |
| `setColorName(index, name)` | Set the name of a color in the palette. |
| `setMaterial(index, property, value)` | Set a material property for a palette color. |
| `setName(name)` | Set the name of the palette. |
| `setSize(count)` | Set the number of colors in the palette. |
| `similar(index, count)` | Find similar colors in the palette. |
| `size()` | Get the number of colors in the palette. |
| `tryAdd(r, g, b, a, skipSimilar)` | Try to add a color to the palette. |
| `warmer(value)` | Make the palette colors warmer. |
| `whiteBalance()` | Apply white balance correction to the palette. |

## Detailed Documentation

### brighter

Make the palette colors brighter.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `factor` | `number` | Brightness factor (optional, default 0.2). |

### changeIntensity

Change the color intensity of the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `scale` | `number` | Intensity scale factor. |

### colder

Make the palette colors colder.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | Cold value (optional, default 10). |

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

### colorName

Get the name of a color in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The name of the color. |

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

### contrastStretching

Apply contrast stretching to the palette.

### copy

Copy a color from one slot to another.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `from` | `integer` | Source color index (0-255). |
| `to` | `integer` | Destination color index (0-255). |

### darker

Make the palette colors darker.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `factor` | `number` | Darkness factor (optional, default 0.2). |

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

### duplicateColor

Duplicate a color to a new slot in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index to duplicate (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The index of the new color slot or -1 if not possible. |

### exchange

Exchange (swap) two colors in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index1` | `integer` | First color index (0-255). |
| `index2` | `integer` | Second color index (0-255). |

### fill

Fill the remaining palette slots with black.

### hasAlpha

Check if a palette color has alpha transparency.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the color has alpha. |

### hasColor

Check if a color exists in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `r` | `integer` | Red component (0-255). |
| `g` | `integer` | Green component (0-255). |
| `b` | `integer` | Blue component (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the color exists in the palette. |

### hasEmit

Check if a palette color is emissive.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the color is emissive. |

### hasFreeSlot

Check if the palette has a free slot for a new color.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if there is a free slot. |

### hasMaterials

Check if the palette has any materials set.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if any materials are set. |

### hash

Get the hash of the palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The palette hash value. |

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

### name

Get the name of the palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The palette name. |

### new

Create a new empty palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `palette` | The newly created palette. |

### reduce

Reduce the palette to a target number of colors.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `targetColors` | `integer` | Target number of colors. |

### removeColor

Remove a color from the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index to remove (0-255). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the color was removed. |

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

### save

Save the palette to a file.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | File path to save to (optional). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the save was successful. |

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

### setColorName

Set the name of a color in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |
| `name` | `string` | The name to set. |

### setMaterial

Set a material property for a palette color.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The color index (0-255). |
| `property` | `string` | The property name. |
| `value` | `number` | The property value. |

### setName

Set the name of the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The new name. |

### setSize

Set the number of colors in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `count` | `integer` | The new color count. |

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

### tryAdd

Try to add a color to the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `r` | `integer` | Red component (0-255). |
| `g` | `integer` | Green component (0-255). |
| `b` | `integer` | Blue component (0-255). |
| `a` | `integer` | Alpha component (0-255, optional, default 255). |
| `skipSimilar` | `boolean` | Skip similar colors (optional, default true). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the color was added. |
| `integer` | The index of the added or matching color. |

### warmer

Make the palette colors warmer.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | Warmth value (optional, default 10). |

### whiteBalance

Apply white balance correction to the palette.


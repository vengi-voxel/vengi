# Font

Global: `g_font`

## Methods

| Method | Description |
| ------ | ----------- |
| `dimensions(text, size)` | Get the width and height of the rendered text in voxels. |
| `new(font)` | Create a new VoxelFont from a TrueType font file. |
| `render(volume, text, x, y, z, size, thickness, color, spacing)` | Render text into a volume at the specified position. |

## Detailed Documentation

### dimensions

Get the width and height of the rendered text in voxels.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `text` | `string` | The text to measure. |
| `size` | `integer` | The font size in pixels (optional, default 16). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The width in voxels. |
| `integer` | The height in voxels. |

### new

Create a new VoxelFont from a TrueType font file.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `font` | `string` | The path to the TrueType font file. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `font` | The created VoxelFont object. |

### render

Render text into a volume at the specified position.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to render into. |
| `text` | `string` | The text to render. |
| `x` | `integer` | The x start position. |
| `y` | `integer` | The y start position. |
| `z` | `integer` | The z start position. |
| `size` | `integer` | The font size in pixels (optional, default 16). |
| `thickness` | `integer` | The thickness in voxels (optional, default 1). |
| `color` | `integer` | The color index (optional, default 0). |
| `spacing` | `integer` | Extra spacing between characters (optional, default 0). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The total advance width in voxels. |


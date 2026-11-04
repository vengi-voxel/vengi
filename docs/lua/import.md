# Import

Global: `g_import`

## Functions

| Function | Description |
| -------- | ----------- |
| `image(filename, stream)` | Load an image from a stream. |
| `imageAsPlane(image, palette, thickness)` | Import an image as a voxel plane. |
| `palette(filename, stream)` | Load a palette from a stream. |
| `scene(filename, stream)` | Import a scene from a file or stream. |

## Detailed Documentation

### image

Load an image from a stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `filename` | `string` | The filename for format detection. |
| `stream` | [`stream`](stream.md) | The stream to read from. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`image`](image.md) | The loaded image. |

### imageAsPlane

Import an image as a voxel plane.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `image` | [`image`](image.md) | The image to convert. |
| `palette` | [`palette`](palette.md) | The palette to use. |
| `thickness` | `integer` | The plane thickness (optional, default 1). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The created node. |

### palette

Load a palette from a stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `filename` | `string` | The filename for format detection. |
| `stream` | [`stream`](stream.md) | The stream to read from. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`palette`](palette.md) | The loaded palette. |

### scene

Import a scene from a file or stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `filename` | `string` | The filename to load. |
| `stream` | [`stream`](stream.md) | Optional stream to read from. |


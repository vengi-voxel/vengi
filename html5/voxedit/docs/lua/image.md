# Image

## Methods

| Method | Description |
| ------ | ----------- |
| `height()` | Get the height of the image in pixels. |
| `name()` | Get the name of the image. |
| `rgba(x, y)` | Get the RGBA color values at the given pixel coordinates. |
| `save(filename)` | Save the image to a file. |
| `width()` | Get the width of the image in pixels. |

## Detailed Documentation

### height

Get the height of the image in pixels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `int` | The image height. |

### name

Get the name of the image.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The image name. |

### rgba

Get the RGBA color values at the given pixel coordinates.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `int` | The x coordinate (0-based). |
| `y` | `int` | The y coordinate (0-based). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `int` | Red value (0-255). |
| `int` | Green value (0-255). |
| `int` | Blue value (0-255). |
| `int` | Alpha value (0-255). |

### save

Save the image to a file.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `filename` | `string` | The filename to save to. |

### width

Get the width of the image in pixels.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `int` | The image width. |


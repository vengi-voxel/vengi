# NormalPalette

Global: `g_normalpalette`

## Methods

| Method | Description |
| ------ | ----------- |
| `hash()` | Get the hash of the normal palette. |
| `load(name)` | Load a normal palette from a file or built-in name. |
| `match(x, y, z)` | Find the closest matching normal in the palette. |
| `name()` | Get the name of the normal palette. |
| `new()` | Create a new empty normal palette. |
| `normal(index)` | Get a normal from the palette as vec3. |
| `save(name)` | Save the normal palette to a file. |
| `setName(name)` | Set the name of the normal palette. |
| `setNormal(index, x, y, z)` | Set a normal in the palette. |
| `size()` | Get the number of normals in the normal palette. |

## Detailed Documentation

### hash

Get the hash of the normal palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The normal palette hash value. |

### load

Load a normal palette from a file or built-in name.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | File path or built-in name (e.g., 'built-in:tiberiansun'). |

### match

Find the closest matching normal in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | X component of the normal. |
| `y` | `number` | Y component of the normal. |
| `z` | `number` | Z component of the normal. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The index of the closest matching normal. |

### name

Get the name of the normal palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The normal palette name. |

### new

Create a new empty normal palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `normalpalette` | The newly created normal palette. |

### normal

Get a normal from the palette as vec3.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The normal index. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `vec3` | The normal direction vector. |

### save

Save the normal palette to a file.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | File path to save to (optional). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if the save was successful. |

### setName

Set the name of the normal palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The new name. |

### setNormal

Set a normal in the palette.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The normal index. |
| `x` | `number` | X component of the normal. |
| `y` | `number` | Y component of the normal. |
| `z` | `number` | Z component of the normal. |

### size

Get the number of normals in the normal palette.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of normals. |


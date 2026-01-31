# Vectors

Global: `g_vec3, g_ivec3, ...`

## Functions

| Function | Description |
| -------- | ----------- |
| `distance(a, b)` | Calculate the distance between two vectors. |
| `dot(a, b)` | Calculate the dot product of two vectors. |
| `length(v)` | Calculate the length (magnitude) of a vector. |
| `new(x, y, z, w)` | Create a new vector with the specified components. |
| `normalize(v)` | Normalize a vector to unit length. |

## Detailed Documentation

### distance

Calculate the distance between two vectors.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `a` | `vec` | The first vector. |
| `b` | `vec` | The second vector. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The Euclidean distance between the two vectors. |

### dot

Calculate the dot product of two vectors.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `a` | `vec` | The first vector. |
| `b` | `vec` | The second vector. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The dot product of the two vectors. |

### length

Calculate the length (magnitude) of a vector.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `v` | `vec` | The vector. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The length of the vector. |

### new

Create a new vector with the specified components.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | The X component. |
| `y` | `number` | The Y component (for vec2 and higher). |
| `z` | `number` | The Z component (for vec3 and higher). |
| `w` | `number` | The W component (for vec4 only). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `vec` | A new vector with the specified components. |

### normalize

Normalize a vector to unit length.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `v` | `vec` | The vector to normalize. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `vec` | The normalized vector with length 1. |


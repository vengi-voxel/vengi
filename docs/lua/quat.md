# Quaternion

Global: `g_quat`

## Functions

| Function | Description |
| -------- | ----------- |
| `new()` | Create a new identity quaternion. |
| `rotateX(angle)` | Create a quaternion rotation around the X axis. |
| `rotateXY(x, y)` | Create a quaternion rotation around X and Y axes. |
| `rotateXYZ(x, z)` | Create a quaternion rotation around X and Z axes. |
| `rotateXZ(x, z)` | Create a quaternion rotation around X and Z axes. |
| `rotateY(angle)` | Create a quaternion rotation around the Y axis. |
| `rotateYZ(y, z)` | Create a quaternion rotation around Y and Z axes. |
| `rotateZ(angle)` | Create a quaternion rotation around the Z axis. |

## Detailed Documentation

### new

Create a new identity quaternion.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | A new identity quaternion. |

### rotateX

Create a quaternion rotation around the X axis.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateXY

Create a quaternion rotation around X and Y axes.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | Rotation angle around X axis in radians. |
| `y` | `number` | Rotation angle around Y axis in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateXYZ

Create a quaternion rotation around X and Z axes.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | Rotation angle around X axis in radians. |
| `z` | `number` | Rotation angle around Z axis in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateXZ

Create a quaternion rotation around X and Z axes.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | Rotation angle around X axis in radians. |
| `z` | `number` | Rotation angle around Z axis in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateY

Create a quaternion rotation around the Y axis.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateYZ

Create a quaternion rotation around Y and Z axes.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `y` | `number` | Rotation angle around Y axis in radians. |
| `z` | `number` | Rotation angle around Z axis in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateZ

Create a quaternion rotation around the Z axis.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |


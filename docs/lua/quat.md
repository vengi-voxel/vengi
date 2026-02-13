# Quaternion

Global: `g_quat`

## Functions

| Function | Description |
| -------- | ----------- |
| `conjugate(q)` | Get the conjugate (inverse rotation) of a quaternion. |
| `fromAxisAngle(axis, angle)` | Create a quaternion from an axis and an angle. |
| `new()` | Create a new identity quaternion. |
| `rotateX(angle)` | Create a quaternion rotation around the X axis (pitch - nod forward/backward). |
| `rotateXY(x, y)` | Create a quaternion rotation around X and Y axes (pitch and yaw). |
| `rotateXYZ(x, y, z)` | Create a quaternion rotation around X, Y, and Z axes (pitch, yaw, roll). |
| `rotateXZ(x, z)` | Create a quaternion rotation around X and Z axes (roll and pitch). |
| `rotateY(angle)` | Create a quaternion rotation around the Y axis (yaw - turn left/right). |
| `rotateYZ(y, z)` | Create a quaternion rotation around Y and Z axes (yaw and roll). |
| `rotateZ(angle)` | Create a quaternion rotation around the Z axis (roll - tilt head left/right). |
| `slerp(a, b, t)` | Spherical linear interpolation between two quaternions. |

## Detailed Documentation

### conjugate

Get the conjugate (inverse rotation) of a quaternion.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `q` | `quat` | The quaternion. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The conjugated quaternion. |

### fromAxisAngle

Create a quaternion from an axis and an angle.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `axis` | `vec3` | The rotation axis (will be normalized). |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### new

Create a new identity quaternion.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | A new identity quaternion. |

### rotateX

Create a quaternion rotation around the X axis (pitch - nod forward/backward).

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateXY

Create a quaternion rotation around X and Y axes (pitch and yaw).

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

Create a quaternion rotation around X, Y, and Z axes (pitch, yaw, roll).

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `x` | `number` | Rotation angle around X axis in radians. |
| `y` | `number` | Rotation angle around Y axis in radians. |
| `z` | `number` | Rotation angle around Z axis in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateXZ

Create a quaternion rotation around X and Z axes (roll and pitch).

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

Create a quaternion rotation around the Y axis (yaw - turn left/right).

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### rotateYZ

Create a quaternion rotation around Y and Z axes (yaw and roll).

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

Create a quaternion rotation around the Z axis (roll - tilt head left/right).

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `angle` | `number` | Rotation angle in radians. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The rotation quaternion. |

### slerp

Spherical linear interpolation between two quaternions.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `a` | `quat` | The start quaternion. |
| `b` | `quat` | The end quaternion. |
| `t` | `number` | Interpolation factor (0.0 to 1.0). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `quat` | The interpolated quaternion. |


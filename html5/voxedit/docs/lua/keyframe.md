# Keyframe

## Methods

| Method | Description |
| ------ | ----------- |
| `frame()` | Get the frame number of this keyframe. |
| `index()` | Get the keyframe index. |
| `interpolation()` | Get the interpolation type. |
| `localOrientation()` | Get the local orientation quaternion. |
| `localScale()` | Get the local scale. |
| `localTranslation()` | Get the local translation. |
| `setInterpolation(type)` | Set the interpolation type. |
| `setLocalOrientation(orientation)` | Set the local orientation. |
| `setLocalScale(scale)` | Set the local scale. |
| `setLocalTranslation(translation)` | Set the local translation. |
| `setWorldOrientation(orientation)` | Set the world orientation. |
| `setWorldScale(scale)` | Set the world scale. |
| `setWorldTranslation(translation)` | Set the world translation. |
| `worldOrientation()` | Get the world orientation quaternion. |
| `worldScale()` | Get the world scale. |
| `worldTranslation()` | Get the world translation. |

## Detailed Documentation

### frame

Get the frame number of this keyframe.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The frame number. |

### index

Get the keyframe index.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The keyframe index. |

### interpolation

Get the interpolation type.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The interpolation type name. |

### localOrientation

Get the local orientation quaternion.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`quat`](quat.md) | The local orientation. |

### localScale

Get the local scale.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`vec3`](vector.md) | The local scale. |

### localTranslation

Get the local translation.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`vec3`](vector.md) | The local translation. |

### setInterpolation

Set the interpolation type.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `type` | `string` | The interpolation type: 'Instant', 'Linear', 'QuadEaseIn', etc. |

### setLocalOrientation

Set the local orientation.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `orientation` | [`quat`](quat.md) | The new local orientation (quaternion or x,y,z,w components). |

### setLocalScale

Set the local scale.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `scale` | [`vec3`](vector.md) | The new local scale. |

### setLocalTranslation

Set the local translation.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `translation` | [`vec3`](vector.md) | The new local translation. |

### setWorldOrientation

Set the world orientation.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `orientation` | [`quat`](quat.md) | The new world orientation (quaternion or x,y,z,w components). |

### setWorldScale

Set the world scale.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `scale` | [`vec3`](vector.md) | The new world scale. |

### setWorldTranslation

Set the world translation.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `translation` | [`vec3`](vector.md) | The new world translation. |

### worldOrientation

Get the world orientation quaternion.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`quat`](quat.md) | The world orientation. |

### worldScale

Get the world scale.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`vec3`](vector.md) | The world scale. |

### worldTranslation

Get the world translation.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`vec3`](vector.md) | The world translation. |


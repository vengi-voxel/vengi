# SceneGraphNode

## Methods

| Method | Description |
| ------ | ----------- |
| `addKeyFrame(frame, interpolation)` | Add a new keyframe at the specified frame. |
| `children()` | Get the child node IDs. |
| `clone()` | Create a copy of the node. |
| `hasKeyFrameForFrame(frame)` | Check if a keyframe exists at the specified frame. |
| `hide()` | Hide the node. |
| `id()` | Get the ID of the node. |
| `isCamera()` | Check if the node is a camera node. |
| `isGroup()` | Check if the node is a group node. |
| `isLocked()` | Check if the node is locked. |
| `isModel()` | Check if the node is a model node. |
| `isPoint()` | Check if the node is a point node. |
| `isReference()` | Check if the node is a model reference node. |
| `isVisible()` | Check if the node is visible. |
| `keyFrame(index)` | Get a keyframe by index. |
| `keyFrameForFrame(frame)` | Get the keyframe for a specific frame number. |
| `lock()` | Lock the node. |
| `name()` | Get the name of the node. |
| `numKeyFrames()` | Get the number of keyframes for the current animation. |
| `palette()` | Get the palette of the node. |
| `parent()` | Get the parent node ID. |
| `pivot()` | Get the normalized pivot point of the node. |
| `property(key)` | Get a custom property from the node. |
| `region()` | Get the region of the model node. |
| `removeKeyFrame(index)` | Remove a keyframe by index. |
| `removeKeyFrameForFrame(frame)` | Remove the keyframe at the specified frame. |
| `setName(name)` | Set the name of the node. |
| `setPalette(palette, remap)` | Set the palette of the node. |
| `setPivot(pivot)` | Set the pivot point of the node. |
| `setProperty(key, value)` | Set a custom property on the node. |
| `show()` | Show the node. |
| `unlock()` | Unlock the node. |
| `uuid()` | Get the UUID of the node. |
| `volume()` | Get the volume of a model node. |

## Detailed Documentation

### addKeyFrame

Add a new keyframe at the specified frame.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `frame` | `integer` | The frame number. |
| `interpolation` | `integer` | Interpolation type (optional, default Linear). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `keyframe` | The newly created keyframe. |

### children

Get the child node IDs.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | A table of child node IDs. |

### clone

Create a copy of the node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The cloned node. |

### hasKeyFrameForFrame

Check if a keyframe exists at the specified frame.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `frame` | `integer` | The frame number. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if keyframe exists. |

### hide

Hide the node.

### id

Get the ID of the node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The node ID. |

### isCamera

Check if the node is a camera node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if this is a camera node. |

### isGroup

Check if the node is a group node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if this is a group node. |

### isLocked

Check if the node is locked.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if node is locked. |

### isModel

Check if the node is a model node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if this is a model node. |

### isPoint

Check if the node is a point node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if this is a point node. |

### isReference

Check if the node is a model reference node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if this is a reference node. |

### isVisible

Check if the node is visible.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if node is visible. |

### keyFrame

Get a keyframe by index.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The keyframe index. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `keyframe` | The keyframe. |

### keyFrameForFrame

Get the keyframe for a specific frame number.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `frame` | `integer` | The frame number. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `keyframe` | The keyframe. |

### lock

Lock the node.

### name

Get the name of the node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The node name. |

### numKeyFrames

Get the number of keyframes for the current animation.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The number of keyframes. |

### palette

Get the palette of the node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `palette` | The node's palette. |

### parent

Get the parent node ID.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The parent node ID. |

### pivot

Get the normalized pivot point of the node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `vec3` | The pivot point (normalized 0-1 range). |

### property

Get a custom property from the node.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `key` | `string` | The property key. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The property value. |

### region

Get the region of the model node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `region` | The node's region. |

### removeKeyFrame

Remove a keyframe by index.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `index` | `integer` | The keyframe index. |

### removeKeyFrameForFrame

Remove the keyframe at the specified frame.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `frame` | `integer` | The frame number. |

### setName

Set the name of the node.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The new name. |

### setPalette

Set the palette of the node.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `palette` | `palette` | The new palette. |
| `remap` | `boolean` | Remap existing colors (optional, default false). |

### setPivot

Set the pivot point of the node.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `pivot` | `vec3` | The new pivot point. |

### setProperty

Set a custom property on the node.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `key` | `string` | The property key. |
| `value` | `string` | The property value. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if property was set successfully. |

### show

Show the node.

### unlock

Unlock the node.

### uuid

Get the UUID of the node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The node UUID. |

### volume

Get the volume of a model node.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `volume` | The volume. |


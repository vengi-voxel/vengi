# SceneGraph

Global: `g_scenegraph`

## Functions

| Function | Description |
| -------- | ----------- |
| `activeAnimation()` | Get the name of the active animation. |
| `addAnimation(name)` | Add a new animation to the scene graph. |
| `align(padding)` | Align all nodes in the scene graph. |
| `animations()` | Get all animation names. |
| `duplicateAnimation(source, target)` | Duplicate an existing animation. |
| `get(id)` | Get a node by its ID. |
| `getByName(name)` | Get a node by its name. |
| `getByUUID(uuid)` | Get a node by its UUID. |
| `hasAnimation(name)` | Check if an animation exists. |
| `new(name, region, visible, type)` | Create a new node in the scene graph. |
| `nodeIds()` | Get all node IDs in the scene graph. |
| `setAnimation(name)` | Set the active animation. |
| `updateTransforms()` | Update all transforms in the scene graph. |

## Detailed Documentation

### activeAnimation

Get the name of the active animation.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The active animation name. |

### addAnimation

Add a new animation to the scene graph.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The animation name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if animation was added successfully. |

### align

Align all nodes in the scene graph.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `padding` | `integer` | Padding between nodes (optional, default 2). |

### animations

Get all animation names.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | A table of animation names. |

### duplicateAnimation

Duplicate an existing animation.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `source` | `string` | The source animation name. |
| `target` | `string` | The new animation name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if animation was duplicated successfully. |

### get

Get a node by its ID.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `id` | `integer` | The node ID (optional, defaults to active node). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The node. |

### getByName

Get a node by its name.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The node name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The node, or nil if not found. |

### getByUUID

Get a node by its UUID.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `uuid` | `string` | The node UUID. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The node, or nil if not found. |

### hasAnimation

Check if an animation exists.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The animation name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if animation exists. |

### new

Create a new node in the scene graph.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The node name. |
| `region` | `region` | The region for model nodes (or boolean for visibility). |
| `visible` | `boolean` | Whether the node is visible (optional, default true). |
| `type` | `string` | Node type: 'Model', 'Group', 'Camera', 'Point' (optional, default 'Group'). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `node` | The newly created node. |

### nodeIds

Get all node IDs in the scene graph.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `table` | Table of node IDs. |

### setAnimation

Set the active animation.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The animation name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if animation was set successfully. |

### updateTransforms

Update all transforms in the scene graph.


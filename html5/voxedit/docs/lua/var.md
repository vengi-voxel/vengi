# Cvar

Global: `g_var`

## Functions

| Function | Description |
| -------- | ----------- |
| `bool(name)` | Get the boolean value of a cvar. |
| `create(name, value, help, nopersist, secret)` | Create a new cvar that is persisted by default. |
| `float(name)` | Get the float value of a cvar. |
| `int(name)` | Get the integer value of a cvar. |
| `setBool(name, value)` | Set the boolean value of a cvar. |
| `setFloat(name, value)` | Set the float value of a cvar. |
| `setInt(name, value)` | Set the integer value of a cvar. |
| `setStr(name, value)` | Set the string value of a cvar. |
| `str(name)` | Get the string value of a cvar. |

## Detailed Documentation

### bool

Get the boolean value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | The cvar's boolean value. |

### create

Create a new cvar that is persisted by default.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |
| `value` | `string` | The initial value. |
| `help` | `string` | Help text (optional). |
| `nopersist` | `boolean` | If true, the cvar won't be persisted (optional). |
| `secret` | `boolean` | If true, the cvar value is hidden (optional). |

### float

Get the float value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The cvar's float value. |

### int

Get the integer value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The cvar's integer value. |

### setBool

Set the boolean value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |
| `value` | `boolean` | The new boolean value. |

### setFloat

Set the float value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |
| `value` | `number` | The new float value. |

### setInt

Set the integer value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |
| `value` | `integer` | The new integer value. |

### setStr

Set the string value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |
| `value` | `string` | The new string value. |

### str

Get the string value of a cvar.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `name` | `string` | The cvar name. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The cvar's string value. |


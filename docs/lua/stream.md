# Stream

## Methods

| Method | Description |
| ------ | ----------- |
| `close()` | Close the stream. |
| `eos()` | Check if end of stream has been reached. |
| `pos()` | Get the current position in the stream (alias for tell). |
| `readDouble()` | Read a 64-bit double (little-endian) from the stream. |
| `readDoubleBE()` | Read a 64-bit double (big-endian) from the stream. |
| `readFloat()` | Read a 32-bit float (little-endian) from the stream. |
| `readFloatBE()` | Read a 32-bit float (big-endian) from the stream. |
| `readInt16()` | Read a signed 16-bit integer (little-endian) from the stream. |
| `readInt16BE()` | Read a signed 16-bit integer (big-endian) from the stream. |
| `readInt32()` | Read a signed 32-bit integer (little-endian) from the stream. |
| `readInt32BE()` | Read a signed 32-bit integer (big-endian) from the stream. |
| `readInt64()` | Read a signed 64-bit integer (little-endian) from the stream. |
| `readInt64BE()` | Read a signed 64-bit integer (big-endian) from the stream. |
| `readInt8()` | Read a signed 8-bit integer from the stream. |
| `readString(terminate)` | Read a string from the stream. |
| `readUInt16()` | Read an unsigned 16-bit integer (little-endian) from the stream. |
| `readUInt16BE()` | Read an unsigned 16-bit integer (big-endian) from the stream. |
| `readUInt32()` | Read an unsigned 32-bit integer (little-endian) from the stream. |
| `readUInt32BE()` | Read an unsigned 32-bit integer (big-endian) from the stream. |
| `readUInt64()` | Read an unsigned 64-bit integer (little-endian) from the stream. |
| `readUInt64BE()` | Read an unsigned 64-bit integer (big-endian) from the stream. |
| `readUInt8()` | Read an unsigned 8-bit integer from the stream. |
| `seek(offset, mode)` | Seek to a position in the stream. |
| `size()` | Get the total size of the stream. |
| `tell()` | Get the current position in the stream. |
| `writeDouble(value)` | Write a 64-bit double (little-endian) to the stream. |
| `writeDoubleBE(value)` | Write a 64-bit double (big-endian) to the stream. |
| `writeFloat(value)` | Write a 32-bit float (little-endian) to the stream. |
| `writeFloatBE(value)` | Write a 32-bit float (big-endian) to the stream. |
| `writeInt16(value)` | Write a signed 16-bit integer (little-endian) to the stream. |
| `writeInt16BE(value)` | Write a signed 16-bit integer (big-endian) to the stream. |
| `writeInt32(value)` | Write a signed 32-bit integer (little-endian) to the stream. |
| `writeInt32BE(value)` | Write a signed 32-bit integer (big-endian) to the stream. |
| `writeInt64(value)` | Write a signed 64-bit integer (little-endian) to the stream. |
| `writeInt64BE(value)` | Write a signed 64-bit integer (big-endian) to the stream. |
| `writeInt8(value)` | Write a signed 8-bit integer to the stream. |
| `writeStream(source)` | Write the contents of another stream to this stream. |
| `writeString(str, terminate)` | Write a string to the stream. |
| `writeUInt16(value)` | Write an unsigned 16-bit integer (little-endian) to the stream. |
| `writeUInt16BE(value)` | Write an unsigned 16-bit integer (big-endian) to the stream. |
| `writeUInt32(value)` | Write an unsigned 32-bit integer (little-endian) to the stream. |
| `writeUInt32BE(value)` | Write an unsigned 32-bit integer (big-endian) to the stream. |
| `writeUInt64(value)` | Write an unsigned 64-bit integer (little-endian) to the stream. |
| `writeUInt64BE(value)` | Write an unsigned 64-bit integer (big-endian) to the stream. |
| `writeUInt8(value)` | Write an unsigned 8-bit integer to the stream. |

## Detailed Documentation

### close

Close the stream.

### eos

Check if end of stream has been reached.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `boolean` | True if at end of stream. |

### pos

Get the current position in the stream (alias for tell).

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The current position. |

### readDouble

Read a 64-bit double (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The value read. |

### readDoubleBE

Read a 64-bit double (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The value read. |

### readFloat

Read a 32-bit float (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The value read. |

### readFloatBE

Read a 32-bit float (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `number` | The value read. |

### readInt16

Read a signed 16-bit integer (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readInt16BE

Read a signed 16-bit integer (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readInt32

Read a signed 32-bit integer (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readInt32BE

Read a signed 32-bit integer (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readInt64

Read a signed 64-bit integer (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readInt64BE

Read a signed 64-bit integer (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readInt8

Read a signed 8-bit integer from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readString

Read a string from the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `terminate` | `boolean` | Whether to stop at null terminator (optional). |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `string` | The string read. |

### readUInt16

Read an unsigned 16-bit integer (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readUInt16BE

Read an unsigned 16-bit integer (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readUInt32

Read an unsigned 32-bit integer (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readUInt32BE

Read an unsigned 32-bit integer (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readUInt64

Read an unsigned 64-bit integer (little-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readUInt64BE

Read an unsigned 64-bit integer (big-endian) from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### readUInt8

Read an unsigned 8-bit integer from the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The value read. |

### seek

Seek to a position in the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `offset` | `integer` | The offset to seek to. |
| `mode` | `integer` | Seek mode (0=SET, 1=CUR, 2=END). |

### size

Get the total size of the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The stream size in bytes. |

### tell

Get the current position in the stream.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `integer` | The current position. |

### writeDouble

Write a 64-bit double (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `number` | The value to write. |

### writeDoubleBE

Write a 64-bit double (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `number` | The value to write. |

### writeFloat

Write a 32-bit float (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `number` | The value to write. |

### writeFloatBE

Write a 32-bit float (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `number` | The value to write. |

### writeInt16

Write a signed 16-bit integer (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeInt16BE

Write a signed 16-bit integer (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeInt32

Write a signed 32-bit integer (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeInt32BE

Write a signed 32-bit integer (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeInt64

Write a signed 64-bit integer (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeInt64BE

Write a signed 64-bit integer (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeInt8

Write a signed 8-bit integer to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeStream

Write the contents of another stream to this stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `source` | `stream` | The source stream to read from. |

### writeString

Write a string to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `str` | `string` | The string to write. |
| `terminate` | `boolean` | Whether to write null terminator (optional). |

### writeUInt16

Write an unsigned 16-bit integer (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeUInt16BE

Write an unsigned 16-bit integer (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeUInt32

Write an unsigned 32-bit integer (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeUInt32BE

Write an unsigned 32-bit integer (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeUInt64

Write an unsigned 64-bit integer (little-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeUInt64BE

Write an unsigned 64-bit integer (big-endian) to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |

### writeUInt8

Write an unsigned 8-bit integer to the stream.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `value` | `integer` | The value to write. |


# IO

Global: `g_io`

## Functions

| Function | Description |
| -------- | ----------- |
| `open(path, mode)` | Open a file from the user's home path for reading or writing. |
| `sysopen(path, mode)` | Open a file from an absolute path or relative to the current working directory. |

## Detailed Documentation

### open

Open a file from the user's home path for reading or writing.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `path` | `string` | The file path relative to the home path. |
| `mode` | `string` | The file mode ('r' for read, 'w' for write). Default is 'r'. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `stream` | A stream object for reading/writing. |

### sysopen

Open a file from an absolute path or relative to the current working directory.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `path` | `string` | The file path to open. |
| `mode` | `string` | The file mode ('r' for read, 'w' for write). Default is 'r'. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `stream` | A stream object for reading/writing. |


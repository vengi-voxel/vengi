# HTTP

Global: `g_http`

## Functions

| Function | Description |
| -------- | ----------- |
| `get(url, headers)` | Perform an HTTP GET request. |
| `post(url, body, headers)` | Perform an HTTP POST request. |

## Detailed Documentation

### get

Perform an HTTP GET request.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `url` | `string` | The URL to request. |
| `headers` | `table` | Optional headers table. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `stream` | Response body as stream. |
| `table` | Response headers. |

### post

Perform an HTTP POST request.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `url` | `string` | The URL to request. |
| `body` | `string` | The request body. |
| `headers` | `table` | Optional headers table. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| `stream` | Response body as stream. |
| `table` | Response headers. |


# HTTP

Global: `g_http`

## Functions

| Function | Description |
| -------- | ----------- |
| `get(url, headers)` | Perform an HTTP GET request. |
| `getAsync(url, headers)` | Start an HTTP GET without blocking. The app dispatches completion each frame. |
| `post(url, body, headers)` | Perform an HTTP POST request. |
| `postAsync(url, body, headers)` | Start an HTTP POST without blocking. The app dispatches completion each frame. |

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
| [`stream`](stream.md) | Response body as stream. |
| `table` | Response headers. |

### getAsync

Start an HTTP GET without blocking. The app dispatches completion each frame.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `url` | `string` | The URL to request. |
| `headers` | `table` | Optional request headers. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`httpRequest`](httprequest.md) | Use ready() and response(). Collection cancels the request. |

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
| [`stream`](stream.md) | Response body as stream. |
| `table` | Response headers. |

### postAsync

Start an HTTP POST without blocking. The app dispatches completion each frame.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `url` | `string` | The URL to request. |
| `body` | `string` | Request body bytes. |
| `headers` | `table` | Optional request headers. |

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`httpRequest`](httprequest.md) | Use ready() and response(). Collection cancels the request. |


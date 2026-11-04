# httpRequest

## Methods

| Method | Description |
| ------ | ----------- |
| `ready()` | Whether the asynchronous request has completed. |
| `response()` | Get the completed response, or no values while pending. |

## Detailed Documentation

### ready

Whether the asynchronous request has completed.

**Returns:**

| Type | Description |
| ---- | ----------- |
| `bool` | True when response() is available. |

### response

Get the completed response, or no values while pending.

**Returns:**

| Type | Description |
| ---- | ----------- |
| [`stream`](stream.md) | Response body, including HTTP error bodies. |
| `table` | Response headers. |
| `int` | HTTP status, or zero on transport failure. |
| `bool` | Transport success; check HTTP status separately. |


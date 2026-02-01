# VoxEdit MCP Server

MCP (Model Context Protocol) server enabling LLMs to control VoxEdit via its network interface.

## MCP Client Configuration

```json
{
  "mcpServers": {
    "voxedit": {
      "command": "/path/to/vengi-voxeditmcp",
      "args": ["-set", "ve_nethostname", "127.0.0.1", "-set", "ve_netport", "10001", "-set", "ve_netrconpassword", "changeme", "-set", "ve_netpassword", "changeme"]
    }
  }
}
```

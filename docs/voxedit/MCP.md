# VoxEdit MCP Server

MCP (Model Context Protocol) server enabling LLMs to control VoxEdit via its network interface.

To use it, you need a running [voxedit](Index.md) instance and start a [server](usage/Network.md).

If you are modifying the settings like the port, the rcon password or anything else, you have to use the same settings for the mcp server, too. See below for `args`.

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

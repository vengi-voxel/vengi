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

## Screenshots

`voxedit_screenshot` renders a node (or the merged visible scene) to a PNG without a GL context via `voxelutil::renderToImage` / `voxelutil::renderIsometricImage`.

Useful arguments:

* `nodeUUID` (optional) - model node to render; omit to merge the whole scene
* `face` - `front`, `back`, `left`, `right`, `up`, `down` (default `front`)
* `isometric` - `true` for an isometric view
* `width` / `height` - optional output size in pixels
* `depthFactor` - optional depth shading for orthographic renders
* `bgR` / `bgG` / `bgB` / `bgA` - background color

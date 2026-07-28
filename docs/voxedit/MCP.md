# VoxEdit MCP Server

MCP (Model Context Protocol) server enabling LLMs to control VoxEdit via its network interface.

The MCP process joins VoxEdit's network as a normal client. Its display name is taken from the MCP
`initialize` `clientInfo.name` (e.g. `cursor-vscode` or `Cursor`), so you can tell IDE and agent
sessions apart in the client list.

To use it, you need a running [voxedit](Index.md) instance and start a [server](usage/Network.md).

## MCP Client Configuration

Print an `mcp.json` document from your current VoxEdit network settings (host, port, passwords)
without starting the UI:

```bash
./vengi-voxedit --mcpjson
./vengi-voxedit --mcpjson > mcp.json
```

Copy the `voxedit` entry into `.cursor/mcp.json` (or redirect stdout as above). Override settings
with `-set` if needed:

```bash
./vengi-voxedit --mcpjson -set ve_netport 10001 -set ve_netrconpassword changeme
```

Example output:

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

If the bind interface is `0.0.0.0` (or another wildcard), the generated hostname is `127.0.0.1` so a
local MCP client can connect.

## Screenshots

`voxedit_screenshot` renders a node (or the merged visible scene) to a PNG without a GL context via `voxelutil::renderToImage` / `voxelutil::renderIsometricImage`.

Large `width`/`height` values upscale the face projection and produce large MCP image payloads. Prefer omitting them (native voxel resolution) unless you need a specific size.

Useful arguments:

* `nodeUUID` (optional) - model node to render; omit to merge the whole scene
* `face` - `front`, `back`, `left`, `right`, `up`, `down` (default `front`)
* `isometric` - `true` for an isometric view
* `width` / `height` - optional output size in pixels
* `depthFactor` - optional depth shading for orthographic renders
* `bgR` / `bgG` / `bgB` / `bgA` - background color

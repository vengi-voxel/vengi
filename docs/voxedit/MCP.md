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

## Building a model

These tools default to the active model node when `nodeUUID` is omitted. Colors are `#RRGGBB` or a palette index. `-1` erases. `0` is palette slot 0. A hex color that is not already in the palette is stored in a free slot (an unused slot when the palette is full). Y is up.

* `voxedit_resize_volume` - set `size` (`[x, y, z]` voxel counts, lower corner stays), an inclusive `region`, or `fit` (plus `padding`) around the existing voxels. Voxels outside the new bounds are cut off. Each axis stays at or below 512.
* `voxedit_draw_slices` - stack text into voxels. `plane` `xz` (default): each slice is one Y layer, each row steps +Z, each character steps +X. `xy`: each slice steps +Z, rows run from the top down. `legend` maps one character to a color. Space and `.` are always empty.
* `voxedit_draw_shape` - `box`, `sphere`, `ellipsoid`, `cylinder` or `line`. Corners are inclusive. `hollow` keeps the shell. Sphere `radius` 4 is 9 voxels across, centered on the given cell.
* `voxedit_place_voxels` - `{x,y,z,idx}`, `{x,y,z,color}` or `[x,y,z,color]`. `0` is palette slot 0. `-1` erases.
* `voxedit_recolor` - replace `fromColor` with `toColor`. `allNodes` walks every model node.
* `voxedit_mirror` - `symmetrize` (default) copies one half onto the other. `flip` turns the model over. `pivot` is the plane where cells `pivot` and `pivot-1` face each other. Omit it to use the middle of the volume. `fromNegative` copies the low side onto the high side.
* `voxedit_shift_voxels` - move every voxel by `[dx, dy, dz]`. Voxels that leave the volume are dropped.
* `voxedit_set_palette_color` - change one slot, including slot 0. Every voxel that uses it changes color. Pass `colorCount` as `index` to append a slot.

`voxedit_shape_brush` and the other brushes are still there for sculpting, painting and selection.

## Screenshots

`voxedit_screenshot` renders a node (or the merged visible scene) to a PNG without a GL context via `scenegraph::renderToImage` / `scenegraph::renderIsometricImage`.

Large `width`/`height` values upscale the face projection and produce large MCP image payloads. Prefer omitting them (native voxel resolution) unless you need a specific size.

Useful arguments:

* `nodeUUID` (optional) - model node to render; omit to merge the whole scene
* `view` - `sheet` (front, left, iso on the top row and back, right, top on the bottom row), or `front`, `back`, `left`, `right`, `top`, `bottom`, `iso`, `iso_left`, `iso_back`. Overrides `face` and `isometric`.
* `size` - sheet width in pixels when `view` is `sheet` (default 768)
* `face` - `front`, `back`, `left`, `right`, `up`, `down` (default `front`)
* `isometric` - `true` for an isometric view
* `width` / `height` - optional output size in pixels
* `depthFactor` - optional depth shading for orthographic renders
* `bgR` / `bgG` / `bgB` / `bgA` - background color

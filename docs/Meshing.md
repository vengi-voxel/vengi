# Meshing

The Vengi voxel engine provides different meshing algorithms to convert voxel data into triangle meshes. Each mesher has different characteristics and produces different visual results.

There are two configuration variables for mesh modes:

- `vox_meshmode` - Used for rendering in the editor (excludes greedy texture, as it's not supported by the renderer)
- `voxformat_meshmode` - Used when exporting/saving mesh formats (supports all mesh types including greedy texture)

> See [configuration](Configuration.md) for more details

## Choosing a Mesher

Quick selection guide:

- **Cubic** - Default choice, good balance of speed and quality
- **Marching Cubes** - Smooth organic shapes, hide voxel structure
- **Binary** - Same as cubic mesher, faster in some situations
- **Texture** - When exporting to texture-based engines/formats (export only, not available for editor rendering)

## Cubic Mesher

**Mode:** `vox_meshmode=0` / `voxformat_meshmode=0`

The cubic surface mesher creates a mesh where each voxel appears as a perfect cube. This is the classic "Minecraft-style" look where voxels maintain their blocky appearance.

## Marching Cubes Mesher

**Mode:** `vox_meshmode=1` / `voxformat_meshmode=1`

Produces smooth, organic-looking meshes by interpolating between voxel colors. This is good for terrains - it doesn't create blocky meshes.

### Best For

- Organic shapes (characters, creatures, plants)
- Smooth terrain
- When you want to hide the voxel structure

### Not Recommended For

- Architectural models (loses sharp edges)
- Pixel art (blurs the aesthetic)
- Hard-surface models (rounds corners)

## Binary Greedy Mesher

**Mode:** `vox_meshmode=2` / `voxformat_meshmode=2`

Like the cubic surface mesher this is creating a "Minecraft-style" look, too.

## Greedy Texture Mesher

**Mode:** `voxformat_meshmode=3` (export only - not available for editor rendering)

Creates a mesh with texture coordinates, packing all visible voxel faces into a single texture atlas. This is useful for exporting to engines or formats that work better with textured meshes.

> **Note:** This mesh mode is only available when exporting/saving mesh formats. It is not supported for in-editor rendering as the renderer does not support textures.

### Visual Characteristics

- Blocky appearance (like cubic mesher)
- Single texture atlas containing all colors

### Limitations

- Texture atlas limited in the max allowed pixels
- Texture packing can fail if atlas fills up

### Best For

- Exporting to game engines
- Formats requiring texture coordinates
- When you need a traditional textured mesh workflow
- Reducing vertices for the mesh

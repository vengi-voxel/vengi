# Voxelization

When converting mesh [formats](Formats.md) (OBJ, FBX, glTF, STL, PLY, etc.) to voxel formats, Vengi uses different voxelization algorithms to convert continuous triangle meshes into discrete voxel grids. The algorithm can be selected using the `voxformat_voxelizemode` configuration variable.

## High Quality Mode (Default)

**Mode:** `voxformat_voxelizemode=0`

This is the recommended voxelization mode that produces the most accurate and visually pleasing results.

### How It Works

1. **Triangle Subdivision:** Large triangles are recursively subdivided using a Sierpinski triangle algorithm. This ensures that even large mesh surfaces are properly sampled and converted to voxels with good coverage.

2. **Axis-Aligned Transformation:** Subdivided triangles are transformed and positioned precisely in the voxel grid using axis-aligned coordinates.

3. **Accurate Sampling:** Each voxel position is carefully evaluated to determine if it should be filled based on the mesh geometry.

### Visual Characteristics

- Accurate representation of the original mesh shape
- Properly handles thin surfaces and fine details
- Good preservation of texture colors and vertex colors
- Consistent results regardless of mesh complexity
- Smooth color sampling from textures and UV coordinates

### Performance Characteristics

- Slower than Fast mode due to triangle subdivision
- Uses more memory during processing (temporary subdivision data)
- Processing time increases significantly with mesh complexity
- Not recommended for extremely large meshes (> 512³ voxels)

### Best For

- Models where accuracy is important
- Detailed meshes with fine features
- Character models and organic shapes
- When texture color sampling is critical
- Models with thin surfaces that need proper representation

### Configuration Options

Additional settings that affect high quality voxelization:

| CVAR | Description | Default |
|------|-------------|---------|
| `voxformat_fillhollow` | Fill the interior of closed meshes | `true` |
| `voxformat_scale` | Uniformly scale the mesh before voxelization | `1.0` |
| `voxformat_scale_x/y/z` | Scale on specific axes | `1.0` |
| `voxformat_rgbweightedaverage` | Average colors based on triangle area contribution | `true` |
| `voxformat_rgbflattenfactor` | Flatten RGB colors when importing (0-255) | `1` |
| `voxformat_createpalette` | Generate palette from mesh colors vs. use existing | `true` |
| `voxformat_gmlregion` | World coordinate region filter for GML/CityGML import (`minX minY minZ maxX maxY maxZ`) | `""` |

## Fast Mode

**Mode:** `voxformat_voxelizemode=1`

A faster voxelization algorithm optimized for speed and memory efficiency. This mode skips triangle subdivision and directly voxelizes the mesh.

### How It Works

1. **Direct Voxelization:** Each triangle is directly rasterized into the voxel grid without subdivision.

2. **Per-Triangle Processing:** Colors and normals are sampled directly from the triangle's UV coordinates and interpolated normals.

3. **Memory Efficient:** Doesn't create temporary subdivision data, keeping memory usage lower.

### Visual Characteristics

- Good results for small to medium triangles
- May have gaps or holes with very large triangles
- Faster color and normal sampling
- Less accurate for meshes with large triangular faces
- May miss thin surfaces if triangles are too large

### Performance Characteristics

- Significantly faster than High Quality mode
- Lower memory usage during processing
- Better suited for large meshes
- Recommended for meshes larger than 512³ voxels

### Best For

- Very large meshes where memory is a concern
- When processing speed is more important than accuracy
- Meshes that already have reasonably sized triangles
- Batch processing of many files
- Preview/draft voxelizations

### Limitations

- Large triangles may not be fully filled
- Can produce gaps in the voxelization
- Less accurate sampling of texture data
- Not recommended for meshes with very large faces

### When to Use

The tool automatically suggests Fast mode when:
- Mesh dimensions exceed 512 voxels in any direction
- Memory is constrained
- Processing time needs to be minimized

You'll see a warning like this if using High Quality mode on large meshes:
```
Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh!
Another option when using very large meshes is to use the fast voxelization mode (voxformat_voxelizemode)
```

## Common Settings

These settings apply to both voxelization modes:

### Scaling

Control the size of the resulting voxel model:

```bash
# Uniformly scale to 2x size
voxconvert -set voxformat_scale 2.0 input.obj output.vox

# Scale differently on each axis
voxconvert -set voxformat_scale_x 2.0 -set voxformat_scale_y 1.0 -set voxformat_scale_z 0.5 input.obj output.vox
```

### Hollow Filling

By default, Vengi fills the interior of closed meshes. To create only surface voxels:

```bash
voxconvert -set voxformat_fillhollow false input.obj output.vox
```

### Color Handling

**Create Palette:** When enabled, automatically generates a palette from the mesh colors:
```bash
voxconvert -set voxformat_createpalette true input.obj output.vox
```

**Weighted Averaging:** When multiple triangles overlap a voxel, average colors based on triangle area:
```bash
voxconvert -set voxformat_rgbweightedaverage true input.obj output.vox
```

### Mesh Simplification

For very complex meshes, enable pre-voxelization simplification:

```bash
voxconvert -set voxformat_mesh_simplify true input.obj output.vox
```

## Choosing a Mode

Quick selection guide:

| Scenario | Recommended Mode | Reason |
|----------|------------------|--------|
| Detailed character models | High Quality | Preserves fine features and thin surfaces |
| Large terrain meshes | Fast | Better memory usage and performance |
| Architectural models | High Quality | Accurate wall and surface representation |
| Preview/draft work | Fast | Quick turnaround time |
| Mesh with large triangles | High Quality | Subdivision ensures proper coverage |
| Batch processing | Fast | Faster overall processing time |
| Dimensions < 256³ | High Quality | Best quality with reasonable performance |
| Dimensions > 512³ | Fast | Necessary for memory constraints |

## Troubleshooting

### Model is too small/large after voxelization

Use the `voxformat_scale` settings to adjust size. See [FAQ](FAQ.md#my-model-is-very-smallbig-after-voxelization) for details.

### No colors after voxelization

Check texture paths and ensure `voxformat_createpalette` is enabled. See [FAQ](FAQ.md#no-colors-after-voxelization---whats-wrong) for details.

### Gaps or holes in voxelized mesh

- Switch to High Quality mode
- Check if `voxformat_fillhollow` is enabled
- Ensure mesh has proper watertight geometry

### Out of memory errors

- Use Fast mode instead of High Quality
- Scale the mesh down using `voxformat_scale`
- Simplify the mesh before voxelization

### Large GML/CityGML datasets

GML and CityGML files often contain large geographic datasets (entire city districts) that would result in enormous voxel regions. If the estimated voxel region after scaling exceeds 1024x256x1024 voxels, a warning is shown.

To limit the import to a specific area, use the `voxformat_gmlregion` cvar to specify a bounding region in GML world coordinates (the same coordinate system used in the source file):

```sh
voxconvert -set voxformat_gmlregion "548000 5930000 0 548500 5930500 100" --input input.gml --output output.vengi
```

Only objects whose geometry is **fully contained** within the specified region are imported. Objects that are partially or fully outside the region are skipped. The region filter is only applied when the estimated voxel size exceeds the threshold — for smaller datasets, all objects are imported regardless of the cvar value.

For more details on configuration, see [Configuration.md](Configuration.md).

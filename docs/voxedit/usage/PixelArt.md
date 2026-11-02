# Pixel art

Open **View -> Pixel art** for a preview of the whole scene. **Mode** switches
between isometric (2:1 dimetric cube stamps) and orthographic **2D**. The image
uses nearest-neighbor sampling. Scroll the mouse wheel over the preview to zoom
one screen pixel per source pixel at a time, so each texel stays a sharp square.
Drag the preview to pan.

Use the **View** combo to switch facing. Isometric offers Front, Right, Back and
Left. 2D also offers Top and Bottom. Every visible model node is included, at
its transform for the current frame. Hidden nodes are skipped. Isometric preview
is skipped when those visible scene bounds exceed `ve_maxsuggestedvolumesize`.

Shared options (also `voxconvert -set` cvars):

* `image_background` - RGBA background (default transparent)
* `image_width` / `image_height` - output size in pixels (`-1` keeps the native size)
* `image_upscale` - allow enlarging when the output size is bigger than native

Isometric mode exposes `image_iso_shading` (Lambert face lighting),
`image_iso_normals` (use a stored voxel normal and the node normal palette
instead of the three cube-face shades; the normal rotates with the node),
`image_iso_materials` (emit, metal, roughness) and `image_iso_transparency`
(palette alpha is blended, and those voxels do not hide the faces behind them).
Voxels without a normal keep the face shades. **Silhouette** inks pixels that touch an empty neighbor before the
image is scaled. The ink color is `image_iso_outline_color`. 2D mode exposes
`image_depthfactor2d` to darken farther voxels.
`image_isometric` selects the projection (default on).

**File -> Save** writes the current preview through the image file dialog.
The default name is the active node name (`House.png`) or `pixelart.png` if
the node has no name. PNG and JPEG are supported.

The [All](ViewModes.md) view mode shows this panel. Other presets hide it until
you enable it from the View menu.

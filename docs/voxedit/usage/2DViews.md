# 2D Views

Open **View -> 2D Views** to edit the selected model as voxel cells. The panel shows
Back (XY), Right (ZY), and Top (XZ) views together. Other nodes and scene transforms
are excluded; coordinates refer to the selected model's volume. Select a model node
with its own volume to edit it. Locked models cannot be edited.

Each view has its own slice slider: Z for Back, X for Right, and Y for Top. All
three views support the same controls:

| Control | Action |
| --- | --- |
| Left click or drag | Draw with the selected palette color |
| Right click or drag | Erase voxels on the selected slice |
| Alt + left click | Pick a voxel's color and normal from the selected slice |
| Mouse wheel | Zoom |
| Middle drag | Pan |
| Fit | Reset zoom and pan |

A stroke changes only the current slice and can be undone or redone as one action.
Existing voxel selections limit edits. **Grid** displays cell boundaries, with red
outlines for occupied cells on the slice. **Background** displays voxels behind
empty slice cells with reduced opacity. Background voxels provide context and are
not edited or picked through the current slice.

Slice positions remain set when the panel is hidden. Selecting another model
centers its slices and resets the views. Resizing a volume clamps slice positions
to its new bounds. The panel is included in the All, Tiberian Sun, and Red Alert 2
view presets.

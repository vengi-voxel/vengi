# 2D Views

Open **View -> 2D Views** to edit the selected model as voxel cells. The panel shows Back (XY), Right (ZY), and Top (XZ) views together. Other nodes and scene transforms are excluded; coordinates refer to the selected model's volume. Select a model node with its own volume to edit it.

Each view has an axis slider to choose its working slice and a **Fit** button to reset zoom and pan. Use the mouse wheel over a canvas to zoom and the middle mouse button to pan. The target cross marks the same voxel position across all three views.

**Background** shows voxels behind the working slice at reduced opacity where the slice itself is empty. Those background voxels provide context; editing still affects the current slice. **Grid** toggles cell borders, which are hidden automatically when the cells are too small to show them clearly.

Model edit bindings apply while the pointer is over a slice canvas. For example, **C** picks the voxel under the target using the default [bindings](../../Configuration.md).

The active brush and all its options apply to a one-voxel-thick working volume. Choose the **Select** brush and its selection mode to select voxels on the current slice. Selected cells have blue outlines. With the Select or Transform brush, drag either of the two small translation handles to move those voxels along the plane's axes.

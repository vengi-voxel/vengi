# 2D Views

Open **View -> 2D Views** to edit the selected model as voxel cells. The panel shows Back (XY), Right (ZY), and Top (XZ) views together. Other nodes and scene transforms are excluded; coordinates refer to the selected model's volume. Select a model node with its own volume to edit it.

Model edit bindings apply while the pointer is over a slice canvas. For example, **C** picks the voxel under the target using the default [bindings](../../Configuration.md).

The active brush and all its options apply to a one-voxel-thick working volume. Choose the **Select** brush and its selection mode to select voxels on the current slice. Selected cells have blue outlines. With the Select or Transform brush, drag either of the two small translation handles to move those voxels along the plane's axes.

/**
 * @file
 */

#pragma once

namespace voxedit {
class SceneManager;

namespace printing {

// Make the model's bottom (min-Y) plane voxel-perfect flat across all bottom nodes.
// See feature_3dprint_flatbase.md for the algorithm.
//   slabThickness: how many voxels above globalMinY to consider as the "base slab"
//                  (default 5). Also used as the bottom-node tolerance.
//   trimPercent:   percent of lowest-Y samples to discard as outliers before picking
//                  the target plane (default 10).
//   debugColor:    palette index to colour newly placed voxels, or -1 to reuse the
//                  sampled column's existing voxel material/colour.
void runFlatBase(SceneManager *sceneMgr, int slabThickness, int trimPercent, int debugColor);

} // namespace printing
} // namespace voxedit

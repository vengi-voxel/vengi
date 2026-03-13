/**
 * @file
 */

#pragma once

#include "voxel/BitVolume.h"
#include "voxel/Face.h"
#include "voxel/SparseVolume.h"

namespace voxel {
class RawVolume;
class Region;
} // namespace voxel

namespace voxelutil {

/**
 * @brief Erode surface voxels based on their solid face-neighbor count.
 *
 * Voxels with fewer solid neighbors are more exposed (protrusions, corners, edges).
 * Strength controls the threshold: 0 removes nothing, 1.0 removes everything except
 * fully surrounded voxels. Each iteration peels one layer at the current threshold.
 * Anchors count as neighbors but are never removed.
 *
 * @param[in,out] solid BitVolume marking solid positions. Eroded positions are cleared.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param[in] anchors Immovable solid positions that participate in neighbor counting.
 * @param strength Erosion strength in [0, 1].
 * @param iterations Number of erosion passes.
 */
void sculptErode(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
				 float strength, int iterations);

/**
 * @brief Grow into air positions adjacent to the surface.
 *
 * Air with more solid neighbors is more "surrounded" (concavity/gap).
 * Strength controls the minimum neighbor count needed to fill: 0 adds nothing,
 * 1.0 fills any air touching a solid voxel. New voxels pick color from the nearest
 * solid face-neighbor, falling back to @p fillVoxel.
 *
 * @param[in,out] solid BitVolume marking solid positions. New positions are set.
 * @param[in,out] voxelMap Color map kept in sync with @p solid. New entries are added.
 * @param[in] anchors Immovable solid positions that participate in neighbor counting.
 * @param strength Growth strength in [0, 1].
 * @param iterations Number of growth passes.
 * @param fillVoxel Fallback voxel when no solid neighbor has a color entry.
 */
void sculptGrow(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
				float strength, int iterations, const voxel::Voxel &fillVoxel);

/**
 * @brief Flatten by peeling layers from the outermost surface along a face normal.
 *
 * Each iteration removes the layer of voxels at the extreme coordinate along the
 * face axis. PositiveY peels from top (removes highest Y layer), NegativeX peels
 * from the left (removes lowest X layer), etc.
 *
 * @param[in,out] solid BitVolume marking solid positions. Peeled positions are cleared.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param face The face direction that determines which axis and side to peel from.
 * @param iterations Number of layers to peel.
 */
void sculptFlatten(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, voxel::FaceNames face, int iterations);

/**
 * @brief Smooth additive: fill height gaps by scanning layers along a face normal.
 *
 * The face defines "up". Scans from bottom to top layer. For each air voxel
 * sitting on a solid voxel, checks the 4 planar neighbors' column heights.
 * If any neighbor column is at least @p heightThreshold voxels taller than the
 * current column, fills the air voxel. Filled voxels become ground for the next
 * layer within the same iteration. Color is picked from the nearest solid neighbor.
 *
 * @param[in,out] solid BitVolume marking solid positions. New positions are set.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param[in] anchors Immovable solid positions included in height computation.
 * @param face The face direction that defines "up".
 * @param heightThreshold Minimum height difference (in voxels) a neighbor column must
 *        exceed the current column for filling to occur. 1 = aggressive, higher = conservative.
 * @param iterations Number of passes. Each pass adds at most one voxel per column.
 * @param fillVoxel Fallback voxel when no solid neighbor has a color entry.
 */
void sculptSmoothAdditive(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
						  voxel::FaceNames face, int heightThreshold, int iterations, const voxel::Voxel &fillVoxel);

/**
 * @brief Smooth erode: remove edge voxels from the top of columns along a face normal.
 *
 * The face defines "up". Scans from top to bottom layer. For each solid voxel
 * that has no solid voxel above it (top of its column) and is on the edge
 * (has fewer than 4 solid planar neighbors at the same height), removes it.
 * Each iteration removes at most one voxel per column.
 *
 * @param[in,out] solid BitVolume marking solid positions. Removed positions are cleared.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param[in] anchors Immovable solid positions included in neighbor checks.
 * @param face The face direction that defines "up".
 * @param iterations Number of passes. Each pass removes at most one voxel per column.
 */
void sculptSmoothErode(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
					   voxel::FaceNames face, int iterations, bool preserveTopHeight = false,
					   int trimPerStep = 1);

/**
 * @brief Erode surface voxels in a volume region.
 *
 * Convenience wrapper that builds BitVolume/SparseVolume from all solid voxels in
 * the region, uses solid voxels adjacent to but outside the region as anchors,
 * runs the erosion algorithm, and writes the result back to the volume.
 *
 * @return The number of voxels removed.
 */
int sculptErode(voxel::RawVolume &volume, const voxel::Region &region, float strength, int iterations);

/**
 * @brief Grow surface voxels in a volume region.
 *
 * @return The number of voxels added.
 */
int sculptGrow(voxel::RawVolume &volume, const voxel::Region &region, float strength, int iterations,
			   const voxel::Voxel &fillVoxel);

/**
 * @brief Flatten voxels in a volume region along a face normal.
 *
 * @return The number of voxels removed.
 */
int sculptFlatten(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face, int iterations);

/**
 * @brief Smooth additive on a volume region along a face normal.
 *
 * @return The number of voxels added.
 */
int sculptSmoothAdditive(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
						 int heightThreshold, int iterations, const voxel::Voxel &fillVoxel);

/**
 * @brief Smooth erode on a volume region along a face normal.
 *
 * @return The number of voxels removed.
 */
int sculptSmoothErode(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face, int iterations,
					  bool preserveTopHeight = false, int trimPerStep = 1);

} // namespace voxelutil

/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "voxel/BitVolume.h"
#include "voxel/Face.h"
#include "voxel/SparseVolume.h"

#include <stdint.h>

namespace palette {
class Palette;
}

namespace voxel {
class RawVolume;
class Region;
} // namespace voxel

namespace voxelutil {

enum class ReskinMode : uint8_t {
	Replace, ///< Skin solid overwrites surface; skin air removes surface voxels
	Blend,   ///< Skin solid overwrites surface; skin air preserves surface voxels
	Negate,  ///< Skin solid removes surface voxels; skin air preserves surface voxels

	Max
};

enum class ReskinFollow : uint8_t {
	None,           ///< Flat plane at max/min surface height
	Median,         ///< Flat plane at median surface height
	Voxel,          ///< Per-column surface following
	CornerAverage,  ///< Bilinear interpolation from 4 corner averages

	Max
};

enum class ReskinRotation : uint8_t {
	R0,   ///< No rotation
	R90,  ///< 90 degrees clockwise
	R180, ///< 180 degrees
	R270, ///< 270 degrees clockwise

	Max
};

enum class ReskinTile : uint8_t {
	Once,    ///< Apply skin once, stop at skin boundary
	Repeat,  ///< Tile across selection, with optional mirroring
	Stretch, ///< Scale skin to fill selection UV extents

	Max
};

/**
 * @brief Configuration for the reskin sculpt operation.
 */
struct ReskinConfig {
	ReskinMode mode = ReskinMode::Blend;
	ReskinFollow follow = ReskinFollow::Voxel;
	ReskinRotation rotation = ReskinRotation::R0;
	ReskinTile tile = ReskinTile::Repeat;
	int offsetU = 0;
	int offsetV = 0;
	int skinDepth = 1;
	/// Vertical offset: positive = skin floats above surface, negative = sinks below
	int zOffset = 0;
	bool invertSkin = false;
	/// Preview mode: only apply a 2x2 tile area for fast feedback
	bool preview = true;
	/// Max repeat count for U tiling (0 = unlimited)
	int maxRepeatU = 0;
	/// Max repeat count for V tiling (0 = unlimited)
	int maxRepeatV = 0;
	/// Which axis of the skin volume is the depth/outward direction (auto-detected from thinnest axis on load)
	math::Axis skinDepthAxis = math::Axis::Y;
};

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
 * @brief Bridge gap: connect boundary voxels by drawing 3D lines between them.
 *
 * Finds all boundary voxels (solid with at least one air face-neighbor) and draws
 * 6-connected lines between every pair, filling air voxels along each line. This
 * bridges gaps and cracks by connecting edges across openings. No face direction
 * needed - works in full 3D.
 *
 * @param[in,out] solid BitVolume marking solid positions. New positions are set.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param[in] anchors Immovable solid positions that participate in boundary detection.
 * @param fillVoxel Fallback voxel when no solid neighbor has a color entry.
 */
void sculptBridgeGap(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
					 const voxel::Voxel &fillVoxel);

/**
 * @brief Smooth gaussian: blur the height map using a 2D Gaussian kernel along a face normal.
 *
 * The face defines "up". Builds a height map of all columns. For each iteration,
 * computes a Gaussian-weighted average of neighbor column heights within the kernel
 * radius. Columns taller than the target are trimmed, columns shorter are filled.
 * Uses circular sampling (only neighbors within the kernel radius are included).
 *
 * @param[in,out] solid BitVolume marking solid positions. Modified to match blurred heights.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param[in] anchors Immovable solid positions included in height computation.
 * @param face The face direction that defines "up".
 * @param kernelSize Radius of the Gaussian kernel (1=3x3, 2=5x5, 3=7x7, 4=9x9).
 * @param sigma Standard deviation of the Gaussian bell curve. Lower = sharper, higher = broader.
 * @param iterations Number of blur passes.
 * @param fillVoxel Fallback voxel when no solid neighbor has a color entry.
 */
void sculptSmoothGaussian(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
						  voxel::FaceNames face, int kernelSize, float sigma, int iterations,
						  const voxel::Voxel &fillVoxel);

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

/**
 * @brief Bridge gap on a volume region by drawing lines between boundary voxels.
 *
 * @return The number of voxels added.
 */
int sculptBridgeGap(voxel::RawVolume &volume, const voxel::Region &region,
					const voxel::Voxel &fillVoxel);

/**
 * @brief Smooth gaussian on a volume region along a face normal.
 *
 * @return The number of voxels changed.
 */
int sculptSmoothGaussian(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
						 int kernelSize, float sigma, int iterations, const voxel::Voxel &fillVoxel);

/**
 * @brief Squash all selected voxels onto a single plane.
 *
 * Projects all solid voxels along the face normal axis onto the specified plane coordinate.
 * For each column: if any solid voxel exists, place one at the plane coordinate using the
 * color of the voxel nearest to the plane. All other voxels in the column are removed.
 *
 * @param[in,out] solid BitVolume marking solid positions.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param face The face direction that defines the column axis.
 * @param planeCoord The coordinate along the face axis where voxels are squashed to.
 */
void sculptSquashToPlane(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, voxel::FaceNames face,
						 int planeCoord);

/**
 * @brief Squash all solid voxels in a volume region onto a single plane.
 *
 * @return The number of voxels changed.
 */
int sculptSquashToPlane(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
						int planeCoord);

/**
 * @brief Reskin: apply a skin volume (texture pattern) onto the selected surface.
 *
 * The face defines the surface normal ("up"). The skin volume's +Z axis maps to the
 * face normal direction. For each (U,V) column in the selection, the algorithm:
 * 1. Finds the surface height along the face normal
 * 2. Erodes erodeDepth layers inward from the surface
 * 3. Applies skinDepth layers of the skin pattern from the original surface inward
 *
 * Tiling, rotation, mirroring, and anchor point control how the skin maps onto the
 * selection's UV plane. ReskinMode controls how skin voxels interact with existing surface.
 *
 * @param[in,out] solid BitVolume marking solid positions.
 * @param[in,out] voxelMap Color map kept in sync with @p solid.
 * @param[in] skin The skin volume to apply. Its +Z axis is treated as the surface normal.
 * @param face The face direction that defines the surface normal.
 * @param config Reskin configuration parameters.
 */
void sculptReskin(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::RawVolume &skin,
				  voxel::FaceNames face, const ReskinConfig &config,
				  const palette::Palette *skinPalette = nullptr,
				  palette::Palette *targetPalette = nullptr);

/**
 * @brief Reskin on a volume region.
 *
 * @return The number of voxels changed.
 */
int sculptReskin(voxel::RawVolume &volume, const voxel::Region &region, const voxel::RawVolume &skin,
				 voxel::FaceNames face, const ReskinConfig &config);

} // namespace voxelutil

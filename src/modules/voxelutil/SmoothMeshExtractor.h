/**
 * @file
 */

#pragma once

namespace voxel {
class RawVolume;
struct ChunkMesh;
} // namespace voxel

namespace voxelutil {

enum class SmoothFilter {
	Laplacian, // Laplacian smoothing - aggressive, converts staircases into slopes. Shrinks the mesh slightly.
	Taubin     // Taubin smoothing - volume-preserving, alternating shrink/inflate. Removes noise without shrinking.
};

static constexpr int SmoothFilterMax = 2;

/**
 * @brief Extract a mesh using marching cubes with optional Taubin/Laplacian mesh smoothing.
 *
 * Converts the voxel volume into a binary density field, runs marching cubes to extract an
 * isosurface, then optionally applies topology-aware mesh smoothing (Laplacian or Taubin)
 * to produce smoother surfaces. Vertex colors are assigned by nearest-voxel lookup.
 *
 * @param volume The input voxel volume
 * @param smoothIterations Number of post-MC smoothing passes (0 = no smoothing, clean manifold mesh only)
 * @param filter The smoothing algorithm to use (Laplacian or Taubin)
 * @param sharpness How much each iteration moves vertices (0.01 = subtle, 0.5 = moderate, 0.9 = aggressive)
 * @param result Output mesh (written to mesh[0])
 */
void extractSmoothMesh(const voxel::RawVolume *volume, int smoothIterations,
					   SmoothFilter filter, float sharpness, voxel::ChunkMesh *result);

} // namespace voxelutil

/**
 * @file
 *
 * https://github.com/cgerikj/binary-greedy-meshing
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include <glm/fwd.hpp>

namespace voxel {

class RawVolume;
class Region;
struct ChunkMesh;

/**
 * @brief Extracts mesh geometry from a voxel volume using binary greedy meshing
 *
 * This function performs high-performance mesh extraction using binary operations
 * combined with greedy meshing. It generates optimized geometry with minimal
 * triangle counts while supporting ambient occlusion.
 *
 * @section extract_requirements Input Requirements
 *
 * The input data includes duplicate edge data from neighboring chunks which is used
 * for visibility culling and AO. For optimal performance, your world data should
 * already be structured this way so that you can feed the data straight into this
 * algorithm.
 *
 * Input data is ordered in YXZ and is 64^3 which results in a 62^3 mesh.
 * The 2-voxel reduction accounts for the 1-voxel border padding on each side.
 *
 * @section extract_output Output
 *
 * Generates two separate meshes stored in the ChunkMesh:
 * - mesh[0]: Opaque geometry (solid voxels)
 * - mesh[1]: Transparent geometry
 *
 * @param volData Source voxel volume to extract from
 * @param region Region of the volume to process (should be chunk-aligned)
 * @param result Output chunk mesh containing opaque and transparent geometry
 * @param translate World space offset applied to all vertex positions
 * @param ambientOcclusion If true, calculates per-vertex AO based on neighboring voxels.
 *                         Disabling AO allows more aggressive quad merging but reduces
 *                         visual quality.
 *
 * @note The chunk size is limited to 62 voxels because we use 64-bit masks with
 *       1-voxel border padding on each side (62 + 2 = 64 bits).
 *
 * @see prepareChunk() for data preparation details
 * @see extractBinaryGreedyMeshType() for the core meshing algorithm
 */
void extractBinaryGreedyMesh(const voxel::RawVolume *volData, const Region &region, ChunkMesh *result,
							 const glm::ivec3 &translate, bool ambientOcclusion = true);

bool exceedsBinaryMesherRegion(const voxel::Region &region);
core::DynamicArray<voxel::Region> getBinaryMesherRegions(const voxel::Region &region);

} // namespace voxel

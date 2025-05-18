/**
 * @file
 *
 * https://github.com/cgerikj/binary-greedy-meshing
 */

#pragma once

#include <glm/fwd.hpp>

namespace voxel {

class RawVolume;
class Region;
struct ChunkMesh;

/**
 * The input data includes duplicate edge data from neighboring chunks which is used for visibility culling and AO. For
 * optimal performance, your world data should already be structured this way so that you can feed the data straight
 * into this algorithm.
 *
 * Input data is ordered in YXZ and is 64^3 which results in a 62^3 mesh.
 */
void extractBinaryGreedyMesh(const voxel::RawVolume *volData, const Region &region, ChunkMesh *result,
							 const glm::ivec3 &translate, bool ambientOcclusion = true);

} // namespace voxel

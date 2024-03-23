/**
 * @file
 */

#pragma once

namespace palette {
class Palette;
}

namespace voxel {
struct ChunkMesh;
class RawVolume;
class Region;
void extractDualContouringMesh(const voxel::RawVolume *volData, const palette::Palette &palette, const Region &region, ChunkMesh *mesh);

} // namespace voxel

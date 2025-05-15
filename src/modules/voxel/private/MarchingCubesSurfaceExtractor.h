/**
 * @file
 */

#pragma once

namespace palette {
class Palette;
}

namespace voxel {

class RawVolume;
class Region;
struct ChunkMesh;

// Also known as: "3D Contouring", "Marching Cubes", "Surface Reconstruction"
void extractMarchingCubesMesh(const RawVolume *volume, const palette::Palette &palette, const Region &region,
							  ChunkMesh *result);

} // namespace voxel

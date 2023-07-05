/**
 * @file
 */

#pragma once

namespace voxel {

class RawVolume;
class Region;
struct ChunkMesh;
class Palette;

// Also known as: "3D Contouring", "Marching Cubes", "Surface Reconstruction"
void extractMarchingCubesMesh(const RawVolume *volume, const Palette &palette, const Region &region, ChunkMesh *result);

} // namespace voxel

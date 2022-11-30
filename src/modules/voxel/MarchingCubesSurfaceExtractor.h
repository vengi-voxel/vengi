/**
 * @file
 */

#pragma once

namespace voxel {

class RawVolume;
class Region;
class Mesh;
class Palette;

// Also known as: "3D Contouring", "Marching Cubes", "Surface Reconstruction"
void extractMarchingCubesMesh(const RawVolume *volume, const Palette &palette, const Region &region, Mesh *result);

} // namespace voxel

#include "TerrainVolumeGenerator.h"

#include "TerrainVolume.h"

namespace Cubiquity {

void generateFloor(TerrainVolume* terrainVolume, int32_t lowerLayerHeight, uint32_t lowerLayerMaterial, int32_t upperLayerHeight, uint32_t upperLayerMaterial) {
	const Region& region = terrainVolume->getEnclosingRegion();

	for (int32_t y = region.getLowerY(); y < region.getUpperY(); y++) {
		// Density decreases with increasing y, to create a floor rather than ceiling
		int32_t density = -y;
		// Add the offset to move the floor to the desired level
		density += upperLayerHeight;
		// 'Compress' the density field so that it changes more quickly
		// from fully empty to fully solid (over only a few voxels)
		density *= 64;
		// Account for the threshold not being at zero
		density += MaterialSet::getMaxMaterialValue() / 2;

		//Clamp resulting density
		density = std::min(density, static_cast<int32_t>(MaterialSet::getMaxMaterialValue()));
		density = std::max(density, 0);

		uint32_t index = (y <= lowerLayerHeight) ? lowerLayerMaterial : upperLayerMaterial;
		MaterialSet material;
		material.setMaterial(index, density);

		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x++) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
				terrainVolume->setVoxel(x, y, z, material, false);
			}
		}
	}

	terrainVolume->markAsModified(region);
}

}

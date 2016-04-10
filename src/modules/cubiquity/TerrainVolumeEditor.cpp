#include "TerrainVolumeEditor.h"

#include "Brush.h"
#include "SmoothSurfaceExtractionTask.h" // FIXME - Shouldn't need this here.
#include "TerrainVolume.h"

using namespace PolyVox;

namespace Cubiquity {
//Note: It would be nice if this function took length squared instead of length.
float computeBrushIntensity(const Brush& brush, float distFromCenter) {
	float lerpFactor = (distFromCenter - brush.innerRadius()) / (brush.outerRadius() - brush.innerRadius());
	float result = lerp(1.0f, 0.0f, lerpFactor);

	result = std::min(result, 1.0f);
	result = std::max(result, 0.0f);

	return result * brush.opacity();
}

void sculptTerrainVolume(TerrainVolume* terrainVolume, const Vector3F& centre, const Brush& brush) {
	int firstX = static_cast<int>(std::floor(centre.getX() - brush.outerRadius()));
	int firstY = static_cast<int>(std::floor(centre.getY() - brush.outerRadius()));
	int firstZ = static_cast<int>(std::floor(centre.getZ() - brush.outerRadius()));

	int lastX = static_cast<int>(std::ceil(centre.getX() + brush.outerRadius()));
	int lastY = static_cast<int>(std::ceil(centre.getY() + brush.outerRadius()));
	int lastZ = static_cast<int>(std::ceil(centre.getZ() + brush.outerRadius()));

	//Check bounds.
	firstX = std::max(firstX, terrainVolume->getEnclosingRegion().getLowerCorner().getX());
	firstY = std::max(firstY, terrainVolume->getEnclosingRegion().getLowerCorner().getY());
	firstZ = std::max(firstZ, terrainVolume->getEnclosingRegion().getLowerCorner().getZ());

	lastX = std::min(lastX, terrainVolume->getEnclosingRegion().getUpperCorner().getX());
	lastY = std::min(lastY, terrainVolume->getEnclosingRegion().getUpperCorner().getY());
	lastZ = std::min(lastZ, terrainVolume->getEnclosingRegion().getUpperCorner().getZ());

	Region region(firstX, firstY, firstZ, lastX, lastY, lastZ);

	::PolyVox::RawVolume<MaterialSet> mSmoothingVolume(region);

	for (int z = firstZ; z <= lastZ; ++z) {
		for (int y = firstY; y <= lastY; ++y) {
			for (int x = firstX; x <= lastX; ++x) {
				Vector3F pos(x, y, z);
				float distFromCentre = (centre - pos).length();

				float intensity = computeBrushIntensity(brush, distFromCentre);

				for (uint32_t matIndex = 0; matIndex < MaterialSet::getNoOfMaterials(); matIndex++) {
					float voxel1nx = static_cast<float>(terrainVolume->getVoxel(x - 1, y, z).getMaterial(matIndex));
					float voxel1px = static_cast<float>(terrainVolume->getVoxel(x + 1, y, z).getMaterial(matIndex));

					float voxel1ny = static_cast<float>(terrainVolume->getVoxel(x, y - 1, z).getMaterial(matIndex));
					float voxel1py = static_cast<float>(terrainVolume->getVoxel(x, y + 1, z).getMaterial(matIndex));

					float voxel1nz = static_cast<float>(terrainVolume->getVoxel(x, y, z - 1).getMaterial(matIndex));
					float voxel1pz = static_cast<float>(terrainVolume->getVoxel(x, y, z + 1).getMaterial(matIndex));

					Vector3DFloat normal(voxel1nx - voxel1px, voxel1ny - voxel1py, voxel1nz - voxel1pz);

					if (normal.length() > 0.001) {
						normal.normalise();
					}

					normal = normal * intensity;

					Vector3F samplePoint = pos - normal;

					int32_t sample = getInterpolatedValue(terrainVolume->_getPolyVoxVolume(), samplePoint).getMaterial(matIndex);

					int32_t iAverage = sample;

					// Prevent wrapping around.
					iAverage = std::min(iAverage, static_cast<int32_t>(MaterialSet::getMaxMaterialValue()));
					iAverage = std::max(iAverage, 1);

					MaterialSet result = mSmoothingVolume.getVoxel(x, y, z);
					result.setMaterial(matIndex, iAverage);
					mSmoothingVolume.setVoxel(x, y, z, result);
				}

				MaterialSet result = mSmoothingVolume.getVoxel(x, y, z);
				result.clampSumOfMaterials();
				mSmoothingVolume.setVoxel(x, y, z, result);
			}
		}
	}

	for (int z = firstZ; z <= lastZ; ++z) {
		for (int y = firstY; y <= lastY; ++y) {
			for (int x = firstX; x <= lastX; ++x) {
				terrainVolume->setVoxel(x, y, z, mSmoothingVolume.getVoxel(x, y, z), false);
			}
		}
	}

	terrainVolume->markAsModified(region);
}

void blurTerrainVolume(TerrainVolume* terrainVolume, const Vector3F& centre, const Brush& brush) {
	int firstX = static_cast<int>(std::floor(centre.getX() - brush.outerRadius()));
	int firstY = static_cast<int>(std::floor(centre.getY() - brush.outerRadius()));
	int firstZ = static_cast<int>(std::floor(centre.getZ() - brush.outerRadius()));

	int lastX = static_cast<int>(std::ceil(centre.getX() + brush.outerRadius()));
	int lastY = static_cast<int>(std::ceil(centre.getY() + brush.outerRadius()));
	int lastZ = static_cast<int>(std::ceil(centre.getZ() + brush.outerRadius()));

	//Check bounds.
	firstX = std::max(firstX, terrainVolume->getEnclosingRegion().getLowerCorner().getX());
	firstY = std::max(firstY, terrainVolume->getEnclosingRegion().getLowerCorner().getY());
	firstZ = std::max(firstZ, terrainVolume->getEnclosingRegion().getLowerCorner().getZ());

	lastX = std::min(lastX, terrainVolume->getEnclosingRegion().getUpperCorner().getX());
	lastY = std::min(lastY, terrainVolume->getEnclosingRegion().getUpperCorner().getY());
	lastZ = std::min(lastZ, terrainVolume->getEnclosingRegion().getUpperCorner().getZ());

	Region region(firstX, firstY, firstZ, lastX, lastY, lastZ);

	::PolyVox::RawVolume<MaterialSet> mSmoothingVolume(region);

	for (int z = firstZ; z <= lastZ; ++z) {
		for (int y = firstY; y <= lastY; ++y) {
			for (int x = firstX; x <= lastX; ++x) {
				Vector3F pos(x, y, z);
				float distFromCentre = (centre - pos).length();

				float intensity = computeBrushIntensity(brush, distFromCentre);

				for (uint32_t matIndex = 0; matIndex < MaterialSet::getNoOfMaterials(); matIndex++) {
					int32_t original = terrainVolume->getVoxel(x, y, z).getMaterial(matIndex);

					int32_t sum = 0;
					sum += terrainVolume->getVoxel(x, y, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x + 1, y, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x - 1, y, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y + 1, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y - 1, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y, z + 1).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y, z - 1).getMaterial(matIndex);

					float fAverage = static_cast<float>(sum) / 7.0f;

					float fLerped = lerp(static_cast<float>(original), fAverage, intensity);

					int32_t iLerped = static_cast<int32_t>(fLerped + 0.5f);

					// Prevent wrapping around.
					iLerped = std::min(iLerped, static_cast<int32_t>(MaterialSet::getMaxMaterialValue()));
					iLerped = std::max(iLerped, 0);

					MaterialSet result = mSmoothingVolume.getVoxel(x, y, z);
					result.setMaterial(matIndex, iLerped);
					mSmoothingVolume.setVoxel(x, y, z, result);
				}

				MaterialSet result = mSmoothingVolume.getVoxel(x, y, z);
				result.clampSumOfMaterials();
				mSmoothingVolume.setVoxel(x, y, z, result);
			}
		}
	}

	for (int z = firstZ; z <= lastZ; ++z) {
		for (int y = firstY; y <= lastY; ++y) {
			for (int x = firstX; x <= lastX; ++x) {
				terrainVolume->setVoxel(x, y, z, mSmoothingVolume.getVoxel(x, y, z), false);
			}
		}
	}

	terrainVolume->markAsModified(region);
}

void blurTerrainVolume(TerrainVolume* terrainVolume, const Region& region) {
	Region croppedRegion = region;
	croppedRegion.cropTo(terrainVolume->getEnclosingRegion());

	::PolyVox::RawVolume<MaterialSet> mSmoothingVolume(croppedRegion);

	for (int z = croppedRegion.getLowerZ(); z <= croppedRegion.getUpperZ(); ++z) {
		for (int y = croppedRegion.getLowerY(); y <= croppedRegion.getUpperY(); ++y) {
			for (int x = croppedRegion.getLowerX(); x <= croppedRegion.getUpperX(); ++x) {
				for (uint32_t matIndex = 0; matIndex < MaterialSet::getNoOfMaterials(); matIndex++) {
					int32_t sum = 0;
					sum += terrainVolume->getVoxel(x, y, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x + 1, y, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x - 1, y, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y + 1, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y - 1, z).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y, z + 1).getMaterial(matIndex);
					sum += terrainVolume->getVoxel(x, y, z - 1).getMaterial(matIndex);

					float fAverage = static_cast<float>(sum) / 7.0f;

					int32_t iAverage = static_cast<int32_t>(fAverage + 0.5f);

					// Prevent wrapping around.
					iAverage = std::min(iAverage, static_cast<int32_t>(MaterialSet::getMaxMaterialValue()));
					iAverage = std::max(iAverage, 0);

					MaterialSet result = mSmoothingVolume.getVoxel(x, y, z);
					result.setMaterial(matIndex, iAverage);
					mSmoothingVolume.setVoxel(x, y, z, result);
				}

				MaterialSet result = mSmoothingVolume.getVoxel(x, y, z);
				result.clampSumOfMaterials();
				mSmoothingVolume.setVoxel(x, y, z, result);
			}
		}
	}

	for (int z = croppedRegion.getLowerZ(); z <= croppedRegion.getUpperZ(); ++z) {
		for (int y = croppedRegion.getLowerY(); y <= croppedRegion.getUpperY(); ++y) {
			for (int x = croppedRegion.getLowerX(); x <= croppedRegion.getUpperX(); ++x) {
				terrainVolume->setVoxel(x, y, z, mSmoothingVolume.getVoxel(x, y, z), false);
			}
		}
	}

	terrainVolume->markAsModified(croppedRegion);
}

void addToMaterial(uint32_t index, uint8_t amountToAdd, MaterialSet& material) {
	uint32_t indexToRemoveFrom = 0; //FIXME - start somewhere random
	uint32_t iterationWithNoRemovals = 0;
	while (amountToAdd > 0) {
		if (indexToRemoveFrom != index) {
			if (material.getMaterial(indexToRemoveFrom) > 0) {
				material.setMaterial(index, material.getMaterial(index) + 1);
				material.setMaterial(indexToRemoveFrom, material.getMaterial(indexToRemoveFrom) - 1);
				amountToAdd--;
				iterationWithNoRemovals = 0;
			} else {
				iterationWithNoRemovals++;
			}
		} else {
			iterationWithNoRemovals++;
		}

		if (iterationWithNoRemovals == MaterialSet::getNoOfMaterials()) {
			break;
		}

		indexToRemoveFrom++;
		indexToRemoveFrom %= MaterialSet::getNoOfMaterials();
	}
}

void paintTerrainVolume(TerrainVolume* terrainVolume, const Vector3F& centre, const Brush& brush, uint32_t materialIndex) {
	int firstX = static_cast<int>(std::floor(centre.getX() - brush.outerRadius()));
	int firstY = static_cast<int>(std::floor(centre.getY() - brush.outerRadius()));
	int firstZ = static_cast<int>(std::floor(centre.getZ() - brush.outerRadius()));

	int lastX = static_cast<int>(std::ceil(centre.getX() + brush.outerRadius()));
	int lastY = static_cast<int>(std::ceil(centre.getY() + brush.outerRadius()));
	int lastZ = static_cast<int>(std::ceil(centre.getZ() + brush.outerRadius()));

	//Check bounds.
	firstX = std::max(firstX, terrainVolume->getEnclosingRegion().getLowerCorner().getX());
	firstY = std::max(firstY, terrainVolume->getEnclosingRegion().getLowerCorner().getY());
	firstZ = std::max(firstZ, terrainVolume->getEnclosingRegion().getLowerCorner().getZ());

	lastX = std::min(lastX, terrainVolume->getEnclosingRegion().getUpperCorner().getX());
	lastY = std::min(lastY, terrainVolume->getEnclosingRegion().getUpperCorner().getY());
	lastZ = std::min(lastZ, terrainVolume->getEnclosingRegion().getUpperCorner().getZ());

	Region region(firstX, firstY, firstZ, lastX, lastY, lastZ);
	for (int z = firstZ; z <= lastZ; ++z) {
		for (int y = firstY; y <= lastY; ++y) {
			for (int x = firstX; x <= lastX; ++x) {
				Vector3F pos(x, y, z);
				float distFromCentre = (centre - pos).length();
				float intensity = computeBrushIntensity(brush, distFromCentre);

				float fAmmountToAdd = intensity * MaterialSet::getMaxMaterialValue();

				int32_t amountToAdd = static_cast<int32_t>(fAmmountToAdd + 0.5f);

				MaterialSet voxel = terrainVolume->getVoxel(x, y, z);
				addToMaterial(materialIndex, amountToAdd, voxel);
				voxel.clampSumOfMaterials();
				terrainVolume->setVoxel(x, y, z, voxel, false);
			}
		}
	}

	terrainVolume->markAsModified(region);
}

}

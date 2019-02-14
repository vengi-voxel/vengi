/**
 * @file
 */
#include "Biome.h"
#include "MaterialColor.h"
#include "Constants.h"

namespace voxel {

Biome::Biome() :
		Biome(VoxelType::Grass, getMaterialIndices(VoxelType::Grass), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false) {
}

Biome::Biome(VoxelType _type, const MaterialColorIndices& _indices, int16_t _yMin, int16_t _yMax, float _humidity, float _temperature, bool _underground) :
		indices(_indices), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(_temperature),
		underground(_underground), type(_type), treeDistribution(calcTreeDistribution()),
		cloudDistribution(calcCloudDistribution()), plantDistribution(calcPlantDistribution()) {
	core_assert(!indices.empty());
}

void Biome::addTreeType(TreeType treeType) {
	_treeTypes.push_back(treeType);
}

int Biome::calcTreeDistribution() const {
	int distribution = 100;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 150;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 200;
	}
	return distribution;
}

int Biome::calcCloudDistribution() const {
	int distribution = 150;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 200;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 250;
	}
	return distribution;
}

int Biome::calcPlantDistribution() const {
	int distribution = 30;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 50;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 100;
	}
	return distribution;
}

}

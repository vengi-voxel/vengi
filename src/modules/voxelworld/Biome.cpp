/**
 * @file
 */
#include "Biome.h"
#include "voxel/MaterialColor.h"
#include "voxel/Constants.h"

namespace voxelworld {

Biome::Biome() :
		Biome(voxel::VoxelType::Grass, 0, voxel::MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, 90, 90, 90, false) {
}

Biome::Biome(voxel::VoxelType _type, int16_t _yMin, int16_t _yMax, float _humidity,
		float _temperature, int _treeDistance, int _cloudDistribution,
		int _plantDistribution, bool _underground) :
		indices(getMaterialIndices(_type)), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(_temperature),
		underground(_underground), type(_type), treeDistance(_treeDistance),
		cloudDistribution(_cloudDistribution), plantDistribution(_plantDistribution) {
	core_assert(!indices.empty());
}

Biome::~Biome() {
	for (const char *t : _treeTypes) {
		SDL_free(const_cast<char*>(t));
	}
	_treeTypes.clear();
}

void Biome::addTreeType(const char *treeType) {
	_treeTypes.push_back(SDL_strdup(treeType));
}

}

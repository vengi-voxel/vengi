/**
 * @file
 */
#pragma once

#include "voxel/MaterialColor.h"
#include "TreeContext.h"
#include "voxel/Voxel.h"
#include "math/Random.h"
#include "core/Assert.h"
#include <glm/common.hpp>
#include <vector>

namespace voxelworld {

class Biome {
private:
	friend class BiomeManager;
	Biome();

	int calcTreeDistribution() const;
	int calcCloudDistribution() const;
	int calcPlantDistribution() const;
	std::vector<TreeType> _treeTypes;

public:
	Biome(voxel::VoxelType type, const voxel::MaterialColorIndices& indices, int16_t yMin, int16_t yMax, float humidity, float temperature, bool underground);

	const voxel::MaterialColorIndices indices;
	const int16_t yMin;
	const int16_t yMax;
	const float humidity;
	const float temperature;
	const bool underground;
	const voxel::VoxelType type;
	const int treeDistribution;
	const int cloudDistribution;
	const int plantDistribution;

	bool hasCactus() const;
	bool hasTrees() const;
	bool hasClouds() const;

	const std::vector<TreeType>& treeTypes() const;
	void addTreeType(TreeType treeType);

	voxel::Voxel voxel(math::Random& random) const;
	voxel::Voxel voxel(uint8_t colorIndex) const;
	voxel::Voxel voxel() const;
};

inline bool Biome::hasCactus() const {
	return temperature > 0.9f || humidity < 0.1f;
}

inline bool Biome::hasTrees() const {
	return temperature > 0.3f && humidity > 0.3f;
}

inline bool Biome::hasClouds() const {
	 return humidity >= 0.5f;
}

inline voxel::Voxel Biome::voxel(math::Random& random) const {
	core_assert(!indices.empty());
	return voxel::Voxel(type, *random.randomElement(indices.begin(), indices.end()));
}

inline voxel::Voxel Biome::voxel(uint8_t colorIndex) const {
	core_assert(!indices.empty());
	const uint8_t max = indices.size() - 1u;
	const uint8_t min = 0u;
	return voxel::Voxel(type, glm::clamp(colorIndex, min, max));
}

inline voxel::Voxel Biome::voxel() const {
	thread_local math::Random random;
	return voxel(random);
}

inline const std::vector<TreeType>& Biome::treeTypes() const {
	return _treeTypes;
}

}

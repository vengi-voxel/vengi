/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Constants.h"
#include "polyvox/Region.h"
#include "polyvox/Voxel.h"
#include "MaterialColor.h"

namespace voxel {

struct Biome {
	constexpr Biome(const Voxel& _voxel, int16_t _yMin, int16_t _yMax,
			float _humidity, float _temperature) :
			voxel(_voxel), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(
					_temperature) {
	}

	Biome() :
			Biome(createRandomColorVoxel(VoxelType::Grass), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f) {
	}
	const Voxel voxel;
	const int16_t yMin;
	const int16_t yMax;
	const float humidity;
	const float temperature;
};

class BiomeManager {
private:
	std::vector<Biome> bioms;

	static const Biome DEFAULT;

public:
	BiomeManager();
	~BiomeManager();

	bool addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type);

	// this lookup must be really really fast - it is executed once per generated voxel
	inline Voxel getVoxel(const glm::ivec3& pos, bool underground = false) const {
		if (underground) {
			return createRandomColorVoxel(VoxelType::Rock);
		}
		const Biome* biome = getBiome(pos);
		return biome->voxel;
	}

	inline Voxel getVoxel(int x, int y, int z, bool underground = false) const {
		return getVoxel(glm::ivec3(x, y, z), underground);
	}

	float getHumidity(const glm::ivec3& pos) const;
	float getTemperature(const glm::ivec3& pos) const;

	bool hasTrees(const glm::ivec3& pos) const;
	int getAmountOfTrees(const Region& region) const;

	const Biome* getBiome(const glm::ivec3& pos) const;

	bool hasClouds(const glm::ivec3& pos) const;

	bool hasPlants(const glm::ivec3& pos) const;
};

}

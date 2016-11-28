/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "Constants.h"
#include "polyvox/Region.h"
#include "polyvox/Voxel.h"
#include "MaterialColor.h"

namespace voxel {

class Biome {
private:
	friend class BiomeManager;
	Biome() :
			Biome(createVoxel(VoxelType::Grass, 0), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false) {
	}

public:
	constexpr Biome(const Voxel& _voxel, int16_t _yMin, int16_t _yMax,
			float _humidity, float _temperature, bool _underground) :
			voxel(_voxel), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(
					_temperature), underground(_underground) {
	}

	const Voxel voxel;
	const int16_t yMin;
	const int16_t yMax;
	const float humidity;
	const float temperature;
	const bool underground;
};

class BiomeManager {
private:
	std::vector<Biome> bioms;

public:
	BiomeManager();
	~BiomeManager();

	bool init();

	bool addBiom(int lower, int upper, float humidity, float temperature, const Voxel& voxel, bool underGround = false);

	// this lookup must be really really fast - it is executed once per generated voxel
	inline Voxel getVoxel(const glm::ivec3& pos, bool underground = false) const {
		core_trace_scoped(BiomeGetVoxel);
		const Biome* biome = getBiome(pos, underground);
		return biome->voxel;
	}

	inline Voxel getVoxel(int x, int y, int z, bool underground = false) const {
		return getVoxel(glm::ivec3(x, y, z), underground);
	}

	float getHumidity(const glm::ivec3& pos) const;
	float getTemperature(const glm::ivec3& pos) const;

	bool hasTrees(const glm::ivec3& pos) const;
	int getAmountOfTrees(const Region& region) const;

	const Biome* getBiome(const glm::ivec3& pos, bool underground = false) const;

	bool hasClouds(const glm::ivec3& pos) const;

	bool hasPlants(const glm::ivec3& pos) const;
};

}

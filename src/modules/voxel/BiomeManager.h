/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "noise/SimplexNoise.h"
#include "Voxel.h"

namespace voxel {

struct Biome {
	constexpr Biome(const Voxel& _voxel, int16_t _yMin, int16_t _yMax,
			float _humidity, float _temperature) :
			voxel(_voxel), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(
					_temperature) {
	}

	constexpr Biome() :
			Biome(createVoxel(Grass1), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f) {
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

	static constexpr Voxel INVALID = createVoxel(Air);
	static constexpr Voxel ROCK = createVoxel(Rock1);
	static constexpr Voxel GRASS = createVoxel(Grass1);
	static constexpr Biome DEFAULT = {}	;

public:
	BiomeManager();
	~BiomeManager();

	bool addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type);

	// this lookup must be really really fast - it is executed once per generated voxel
	inline const Voxel& getVoxelType(const glm::ivec3& pos, bool underground = false, float noise = 1.0f) const {
		if (underground) {
			return ROCK;
		}
		const Biome* biome = getBiome(pos, noise);
		return biome->voxel;
	}

	inline const Voxel& getVoxelType(int x, int y, int z, bool underground = false, float noise = 1.0f) const {
		return getVoxelType(glm::ivec3(x, y, z), underground, noise);
	}

	bool hasTrees(const glm::ivec3& pos, float noise = 1.0f) const;

	const Biome* getBiome(const glm::ivec3& pos, float noise = 1.0f) const;

	bool hasClouds(const glm::ivec3& pos, float noise = 1.0f) const;
};

}

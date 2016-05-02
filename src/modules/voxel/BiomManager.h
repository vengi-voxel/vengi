#pragma once

#include "core/Common.h"
#include "Voxel.h"

namespace voxel {

struct Biome {
	Voxel voxel;
	int16_t yMin;
	int16_t yMax;
	float humidity;
	float temperature;
};

class BiomManager {
private:
	Biome bioms[MAX_HEIGHT];

	const Voxel INVALID = createVoxel(Air);
	const Voxel ROCK = createVoxel(Rock);

public:
	BiomManager();

	bool addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type);

	// this lookup must be really really fast - it is executed once per generated voxel
	inline const Voxel& getVoxelType(const glm::ivec3& pos, bool cave = false, float noise = 1.0f) const {
		if (pos.y < 0 || pos.y >= MAX_HEIGHT) {
			return INVALID;
		}
		if (cave) {
			return ROCK;
		}
		core_assert(noise >= 0.0f && noise <= 1.0f);
		return getBiome(pos, noise).voxel;
	}

	inline const Voxel& getVoxelType(int x, int y, int z, bool cave = false, float noise = 1.0f) const {
		return getVoxelType(glm::ivec3(x, y, z), cave, noise);
	}

	inline bool hasTrees(const glm::ivec3& pos, float noise = 1.0f) const {
		if (pos.y < MAX_WATER_HEIGHT) {
			return false;
		}
		if (pos.y > MAX_TERRAIN_HEIGHT) {
			return false;
		}
		const Biome& biome = getBiome(pos, noise);
		if (biome.voxel.getMaterial() != Grass) {
			return false;
		}
		return biome.temperature > 0.3f && biome.humidity > 0.3f;
	}

	inline const Biome& getBiome(const glm::ivec3& pos, float noise = 1.0f) const {
		return bioms[glm::clamp(int(pos.y * noise), 0, MAX_TERRAIN_HEIGHT- 1)];
	}

	inline bool hasClouds(const glm::ivec3& pos, float noise = 1.0f) const {
		if (pos.y <= MAX_TERRAIN_HEIGHT) {
			return false;
		}
		core_assert(noise >= 0.0f && noise <= 1.0f);
		return getBiome(pos, noise).humidity >= 0.5f;
	}
};

}

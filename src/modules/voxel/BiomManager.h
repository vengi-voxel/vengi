#pragma once

#include "core/Common.h"
#include "Voxel.h"

namespace voxel {

class BiomManager {
private:
	Voxel bioms[MAX_HEIGHT];

	const Voxel INVALID = createVoxel(Air);
	const Voxel ROCK = createVoxel(Rock);

public:
	BiomManager() {
		for (int i = 0; i < MAX_HEIGHT; ++i) {
			bioms[i] = createVoxel(Grass);
		}
	}

	bool addBiom(int lower, int upper, const Voxel& type) {
		if (lower < 0 || lower >= MAX_HEIGHT || upper <= lower || upper < 0 || upper >= MAX_HEIGHT) {
			return false;
		}
		for (int i = lower; i < upper; ++i) {
			bioms[i] = type;
		}
		return true;
	}

	// this lookup must be really really fast - it is executed once per generated voxel
	inline const Voxel& getVoxelType(const glm::ivec3& pos, bool cave = false, float noise = 1.0f) const {
		if (pos.y < 0 || pos.y >= MAX_HEIGHT) {
			return INVALID;
		}
		if (cave) {
			return ROCK;
		}
		core_assert(noise >= 0.0f && noise <= 1.0f);
		return bioms[glm::clamp(int(pos.y * noise), 0, MAX_HEIGHT - 1)];
	}

	inline const Voxel& getVoxelType(int x, int y, int z, bool cave = false, float noise = 1.0f) const {
		return getVoxelType(glm::ivec3(x, y, z), cave, noise);
	}

	inline bool hasTrees(const glm::ivec3& pos) const {
		if (pos.y < MAX_WATER_HEIGHT) {
			return false;
		}
		if (pos.y > MAX_TERRAIN_HEIGHT) {
			return false;
		}
		return getVoxelType(pos, false, 1.0f).getMaterial() == Grass;
	}

	inline bool hasClouds(const glm::ivec3& pos) const {
		return pos.y > MAX_TERRAIN_HEIGHT;
	}
};

}

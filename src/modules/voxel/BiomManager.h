#pragma once

#include "core/Common.h"
#include <glm/glm.hpp>
#include "polyvox/Voxel.h"

namespace voxel {

class BiomManager {
private:
	Voxel bioms[MAX_HEIGHT];

	const Voxel INVALID = createVoxel(Air);

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
	inline Voxel getVoxelType(const glm::ivec3& pos, float noise = 1.0f) const {
		if (pos.y < 0 || pos.y >= MAX_HEIGHT) {
			return INVALID;
		}
		core_assert(noise >= 0.0f && noise <= 1.0f);
		return bioms[glm::clamp(int(pos.y * noise), 0, MAX_HEIGHT - 1)];
	}

	inline Voxel getVoxelType(int x, int y, int z, float noise = 1.0f) const {
		return getVoxelType(glm::ivec3(x, y, z), noise);
	}

	inline bool hasTrees(const glm::ivec3& pos) const {
		return getVoxelType(pos, 1.0f).getMaterial() == Grass;
	}

	inline bool hasClouds(const glm::ivec3& pos) const {
		return pos.y > MAX_HEIGHT - 70;
	}
};

}

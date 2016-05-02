#include "BiomManager.h"

namespace voxel {

BiomManager::BiomManager() {
	for (int i = 0; i < MAX_TERRAIN_HEIGHT; ++i) {
		bioms[i] = Biome { createVoxel(Grass), 0, MAX_TERRAIN_HEIGHT, 0.5f, 0.5f };
	}
}

bool BiomManager::addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type) {
	if (lower < 0 || lower >= MAX_HEIGHT || upper <= lower || upper < 0 || upper >= MAX_HEIGHT) {
		return false;
	}
	for (int i = lower; i < upper; ++i) {
		bioms[i] = Biome { type, int16_t(lower), int16_t(upper), humidity, temperature };
	}
	return true;
}

}

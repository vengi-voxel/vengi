#include "BiomeManager.h"

namespace voxel {

BiomeManager::BiomeManager() {
	for (int i = 0; i < int(SDL_arraysize(bioms)); ++i) {
		bioms[i] = Biome { GRASS, 0, MAX_TERRAIN_HEIGHT, 0.5f, 0.5f, nullptr };
	}
}

bool BiomeManager::addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type) {
	if (lower < 0 || lower >= int(SDL_arraysize(bioms)) || upper <= lower || upper < 0 || upper >= int(SDL_arraysize(bioms))) {
		return false;
	}
	for (int i = lower; i < upper; ++i) {
		const Biome b { type, int16_t(lower), int16_t(upper), humidity, temperature, &bioms[i] };
		bioms[i] = b;
	}
	return true;
}

}

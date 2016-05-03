#include "BiomeManager.h"

namespace voxel {

constexpr Voxel BiomeManager::INVALID;
constexpr Voxel BiomeManager::ROCK;
constexpr Voxel BiomeManager::GRASS;
constexpr Biome BiomeManager::DEFAULT;

BiomeManager::BiomeManager() {
	for (int i = 0; i < int(SDL_arraysize(bioms)); ++i) {
		bioms[i] = new Biome();
	}
}

BiomeManager::~BiomeManager() {
	for (int i = 0; i < int(SDL_arraysize(bioms)); ++i) {
		for (Biome *b = bioms[i]; b != nullptr; ) {
			Biome* toDelete = b;
			b = b->next;
			delete toDelete;
		}
	}
}

bool BiomeManager::addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type) {
	if (lower < 0 || lower >= int(SDL_arraysize(bioms)) || upper <= lower || upper < 0 || upper > int(SDL_arraysize(bioms))) {
		return false;
	}
	for (int i = lower; i < upper; ++i) {
		Biome* b = new Biome(type, int16_t(lower), int16_t(upper), humidity, temperature, bioms[i]);
		bioms[i] = b;
	}
	return true;
}

}

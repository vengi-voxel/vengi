#include "BiomeManager.h"

namespace voxel {

constexpr Voxel BiomeManager::INVALID;
constexpr Voxel BiomeManager::ROCK;
constexpr Voxel BiomeManager::GRASS;
constexpr Biome BiomeManager::DEFAULT;

BiomeManager::BiomeManager() {
}

BiomeManager::~BiomeManager() {
}

bool BiomeManager::addBiom(int lower, int upper, float humidity, float temperature, const Voxel& type) {
	bioms.emplace_back(type, int16_t(lower), int16_t(upper), humidity, temperature);
	return true;
}

}

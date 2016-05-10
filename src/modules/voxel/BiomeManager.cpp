/**
 * @file
 */

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

float BiomeManager::getHumidity(const glm::ivec3& pos) const {
	const glm::vec2 noisePos(pos.x, pos.z);
	const float n = noise::Simplex::Noise2D(noisePos, 1, 1.0f, 1.0f, 1.0f);
	return noise::norm(n);
}

float BiomeManager::getTemperature(const glm::ivec3& pos) const {
	const glm::vec2 noisePos(pos.x, pos.z);
	// TODO: apply y value
	const float n = noise::Simplex::Noise2D(noisePos, 1, 1.2f, 1.2f, 1.2f);
	return noise::norm(n);
}

const Biome* BiomeManager::getBiome(const glm::ivec3& pos) const {
	const float humidity = getHumidity(pos);
	const float temperature = getTemperature(pos);

	const Biome *biomeBestMatch = &DEFAULT;
	float distMin = std::numeric_limits<float>::max();

	for (const Biome& biome : bioms) {
		if (pos.y > biome.yMax || pos.y < biome.yMin) {
			continue;
		}
		const float dTemperature = temperature - biome.temperature;
		const float dHumidity = humidity - biome.humidity;
		const float dist = (dTemperature * dTemperature) + (dHumidity * dHumidity);
		if (dist < distMin) {
			biomeBestMatch = &biome;
			distMin = dist;
		}
	}
	return biomeBestMatch;
}

bool BiomeManager::hasTrees(const glm::ivec3& pos) const {
	if (pos.y < MAX_WATER_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	if (!isGrass(biome->voxel.getMaterial())) {
		return false;
	}
	return biome->temperature > 0.3f && biome->humidity > 0.3f;
}

bool BiomeManager::hasClouds(const glm::ivec3& pos) const {
	if (pos.y <= MAX_MOUNTAIN_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	return biome->humidity >= 0.6f;
}

}

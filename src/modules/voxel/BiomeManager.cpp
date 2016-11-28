/**
 * @file
 */

#include "BiomeManager.h"
#include "noise/SimplexNoise.h"

namespace voxel {

static const Biome& getDefault() {
	static const Biome biome(createRandomColorVoxel(VoxelType::Grass), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false);
	return biome;
}

BiomeManager::BiomeManager() {
}

BiomeManager::~BiomeManager() {
}

bool BiomeManager::init() {
	return true;
}

bool BiomeManager::addBiom(int lower, int upper, float humidity, float temperature, const Voxel& voxel, bool underGround) {
	bioms.emplace_back(voxel, int16_t(lower), int16_t(upper), humidity, temperature, underGround);
	return true;
}

float BiomeManager::getHumidity(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeGetHumidity);
	const glm::vec2 noisePos(pos.x, pos.z);
	const float n = noise::Simplex::Noise2D(noisePos, 1, 1.0f, 0.001f, 1.0f);
	return noise::norm(n);
}

float BiomeManager::getTemperature(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeGetTemperature);
	const glm::vec2 noisePos(pos.x, pos.z);
	// TODO: apply y value
	// const float scaleY = pos.y / (float)MAX_HEIGHT;
	const float n = noise::Simplex::Noise2D(noisePos, 1, 1.2f, 0.01f, 1.2f);
	return noise::norm(n);
}

const Biome* BiomeManager::getBiome(const glm::ivec3& pos, bool underground) const {
	core_trace_scoped(BiomeGetBiome);
	const float humidity = getHumidity(pos);
	const float temperature = getTemperature(pos);

	const Biome *biomeBestMatch = &getDefault();
	float distMin = std::numeric_limits<float>::max();

	core_trace_scoped(BiomeGetBiomeLoop);
	for (const Biome& biome : bioms) {
		if (pos.y > biome.yMax || pos.y < biome.yMin || biome.underground != underground) {
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

int BiomeManager::getAmountOfTrees(const Region& region) const {
	core_trace_scoped(BiomeGetAmountOfTrees);
	const glm::ivec3&pos = region.getCentre();
	const Biome* biome = getBiome(pos);
	const int maxDim = region.getDepthInCells();
	if (biome->temperature > 0.7f || biome->humidity < 0.2f) {
		return maxDim / 16;
	}
	if (biome->temperature > 0.9f || biome->humidity < 0.1f) {
		return maxDim / 32;
	}
	return maxDim / 6;
}

bool BiomeManager::hasTrees(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasTrees);
	if (pos.y < MAX_WATER_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	if (!isGrass(biome->voxel.getMaterial())) {
		return false;
	}
	if (biome->temperature > 0.9f || biome->humidity < 0.1f) {
		return false;
	}
	return biome->temperature > 0.3f && biome->humidity > 0.3f;
}

bool BiomeManager::hasClouds(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasClouds);
	if (pos.y <= MAX_MOUNTAIN_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	return biome->humidity >= 0.5f;
}

bool BiomeManager::hasPlants(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasPlants);
	// TODO:
	return hasTrees(pos);
}

}

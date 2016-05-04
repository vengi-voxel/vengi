#pragma once

#include "core/Common.h"
#include "noise/SimplexNoise.h"
#include "Voxel.h"

namespace voxel {

struct Biome {
	constexpr Biome(const Voxel& _voxel, int16_t _yMin, int16_t _yMax,
			float _humidity, float _temperature, Biome* _next = nullptr) :
			voxel(_voxel), yMin(_yMin), yMax(_yMax), humidity(_humidity), temperature(
					_temperature), next(_next) {
	}

	constexpr Biome() :
			Biome(createVoxel(Grass1), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f) {
	}
	const Voxel voxel;
	const int16_t yMin;
	const int16_t yMax;
	const float humidity;
	const float temperature;
	Biome* next;
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
		core_assert(noise >= 0.0f && noise <= 1.0f);
		const Biome* biome = getBiome(pos, noise);
		return biome->voxel;
	}

	inline const Voxel& getVoxelType(int x, int y, int z, bool underground = false, float noise = 1.0f) const {
		return getVoxelType(glm::ivec3(x, y, z), underground, noise);
	}

	inline bool hasTrees(const glm::ivec3& pos, float noise = 1.0f) const {
		if (pos.y < MAX_WATER_HEIGHT) {
			return false;
		}
		const Biome* biome = getBiome(pos, noise);
		if (!isGrass(biome->voxel.getMaterial())) {
			return false;
		}
		return biome->temperature > 0.3f && biome->humidity > 0.3f;
	}

	inline const Biome* getBiome(const glm::ivec3& pos, float noise = 1.0f) const {
		core_assert_msg(noise >= 0.0f && noise <= 1.0f, "noise must be normalized [-1.0,1.0]: %f", noise);
		const glm::vec4 noisePos(pos.x, pos.y, pos.z, noise);
		// TODO: reasonable values
		const float humidityNoise = noise::Simplex::Noise4D(noisePos, 1, 1.0f, 1.0f, 1.0f);
		const float temperatureNoise = noise::Simplex::Noise4D(noisePos, 1, 1.2f, 1.2f, 1.2f);
		const float humidityNoiseNorm = noise::norm(humidityNoise);
		const float temperatureNoiseNorm = noise::norm(temperatureNoise);

		const Biome *biomeBestMatch = &DEFAULT;
		float distMin = std::numeric_limits<float>::max();

		for (const Biome& biome : bioms) {
			if (pos.y > biome.yMax || pos.y < biome.yMin) {
				continue;
			}
			const float dTemperature = temperatureNoiseNorm - biome.temperature;
			const float dHumidity = humidityNoiseNorm - biome.humidity;
			const float dist = (dTemperature * dTemperature) + (dHumidity * dHumidity);
			if (dist < distMin) {
				biomeBestMatch = &biome;
				distMin = dist;
			}
		}
		return biomeBestMatch;
	}

	inline bool hasClouds(const glm::ivec3& pos, float noise = 1.0f) const {
		if (pos.y <= MAX_MOUNTAIN_HEIGHT) {
			return false;
		}
		const Biome* biome = getBiome(pos, noise);
		return biome->humidity >= 0.6f;
	}
};

}

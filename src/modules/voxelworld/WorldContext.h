/**
 * @file
 */

#pragma once

#include <string>

namespace voxelworld {

/**
 * @brief Defines how the world is generated
 */
struct WorldContext {
	WorldContext();
	bool load(const std::string& lua);

	int landscapeNoiseOctaves;
	float landscapeNoiseLacunarity;
	float landscapeNoiseFrequency;
	float landscapeNoiseGain;

	int caveNoiseOctaves;
	float caveNoiseLacunarity;
	float caveNoiseFrequency;
	float caveNoiseGain;
	float caveDensityThreshold;

	int mountainNoiseOctaves;
	float mountainNoiseLacunarity;
	float mountainNoiseFrequency;
	float mountainNoiseGain;
};

}

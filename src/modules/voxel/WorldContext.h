/**
 * @file
 */

#pragma once

#include <string>

namespace voxel {

struct WorldContext {
	WorldContext();
	bool load(const std::string& lua);

	int landscapeNoiseOctaves;
	float landscapeNoisePersistence;
	float landscapeNoiseFrequency;
	float landscapeNoiseAmplitude;

	int caveNoiseOctaves;
	float caveNoisePersistence;
	float caveNoiseFrequency;
	float caveNoiseAmplitude;
	float caveDensityThreshold;

	int mountainNoiseOctaves;
	float mountainNoisePersistence;
	float mountainNoiseFrequency;
	float mountainNoiseAmplitude;
};

}

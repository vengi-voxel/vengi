/**
 * @file
 */

#pragma once

namespace voxel {

struct WorldContext {
	WorldContext();

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

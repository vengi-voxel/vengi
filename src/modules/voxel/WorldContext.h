/**
 * @file
 */

#pragma once

#include "io/File.h"

namespace voxel {

struct WorldContext {
	WorldContext();
	bool load(const io::FilePtr& luaFile);

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

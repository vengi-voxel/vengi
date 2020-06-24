/**
 * @file
 */

#pragma once

#include "NoiseType.h"
#include "video/Texture.h"
#include <stdint.h>

struct NoiseData {
	float separation = 5.0f; // poisson
	float frequency = 0.0f;
	float offset = 0.0f;
	float lacunarity = 0.0f;
	int octaves = 0;
	float gain = 0.0f;
	uint64_t millis = 0;
	uint64_t endmillis = 0;
	int seed = 0;
	float ridgedOffset = 1.0f; // ridged noise
	NoiseType noiseType = NoiseType::Max;
	bool enableDistance = false; // voronoi

	video::TexturePtr graph;
	video::TexturePtr noise;
};

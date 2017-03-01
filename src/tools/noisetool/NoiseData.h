#pragma once

#include "NoiseType.h"

struct NoiseData {
	float frequency = 0.0f;
	float offset = 0.0f;
	float lacunarity = 0.0f;
	int octaves = 0;
	float gain = 0.0f;
	unsigned long millis = 0l;
	unsigned long endmillis = 0l;
	NoiseType noiseType = NoiseType::Max;

	tb::TBImage graph;
	tb::TBImage noise;
};

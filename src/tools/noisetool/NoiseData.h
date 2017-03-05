#pragma once

#include "NoiseType.h"

struct NoiseData {
	float separation = 5.0f; // poisson
	float frequency = 0.0f;
	float offset = 0.0f;
	float lacunarity = 0.0f;
	int octaves = 0;
	float gain = 0.0f;
	unsigned long millis = 0l;
	unsigned long endmillis = 0l;
	int seed = 0;
	float ridgedOffset = 1.0f; // ridged noise
	NoiseType noiseType = NoiseType::Max;
	bool enableDistance = false; // voronoi

	tb::TBImage graph;
	tb::TBImage noise;
};

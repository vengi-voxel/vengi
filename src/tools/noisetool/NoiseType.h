#pragma once

enum class NoiseType {
	simplexNoise,
	ridgedNoise,
	flowNoise,
	fbm,
	fbmCascade,
	fbmAnalyticalDerivatives,
	flowNoiseFbm,
	ridgedMFTime,
	ridgedMF,
	ridgedMFCascade,
	ridgedMFScaled,
	iqNoise,
	iqNoiseScaled,
	analyticalDerivatives,
	noiseCurlNoise,
	worleyNoise,
	voronoi,

	Max
};

extern const char* getNoiseTypeName(NoiseType t);

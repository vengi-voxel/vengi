#pragma once

enum class NoiseType {
	doubleNoise,
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
	iqNoise,
	analyticalDerivatives,
	noiseCurlNoise,
	worleyNoise,
	worleyNoiseFbm,
	voronoi,
	swissTurbulence,
	jordanTurbulence,
	poissonDiskDistribution,

	Max
};

extern const char* getNoiseTypeName(NoiseType t);

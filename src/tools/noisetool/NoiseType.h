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
	iqNoiseScaled,
	analyticalDerivatives,
	noiseCurlNoise,
	worleyNoise,
	worleyNoiseFbm,
	voronoi,
	swissTurbulence,
	jordanTurbulence,

	Max
};

extern const char* getNoiseTypeName(NoiseType t);

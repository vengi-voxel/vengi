/**
 * @file
 */

#pragma once

#include "NNode.h"
#include "noise/Noise.h"

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

	Max
};

class NoiseNode: public NNode {
protected:
	float frequency = 0.001f;
	float offset = 0.0f;
	float lacunarity = 2.0f;
	int octaves = 4;
	float gain = 0.5f;
	int noiseTypeIndex = (int)NoiseType::simplexNoise;
	noise::Noise _noise;

	static bool GetNoiseTypeFromEnumIndex(void*, int value, const char** pTxt);

	float getNoise(int x, int y, int z) override;

	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;
public:
	static NoiseNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

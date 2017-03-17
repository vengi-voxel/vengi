#pragma once

#include "Node.h"

namespace ImGui {

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

	ImGui::NodeGraphEditor* nge = nullptr;

	static bool GetNoiseTypeFromEnumIndex(void*, int value, const char** pTxt);

	float getNoise(int x, int y) override;

	const char* getTooltip() const override;

	const char* getInfo() const override;

	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;

	void onEdited() override;
public:
	static NoiseNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

}

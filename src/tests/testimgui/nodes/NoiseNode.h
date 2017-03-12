#pragma once

#include "Node.h"

namespace ImGui {

class NoiseNode: public NNode {
protected:
	float frequency = 0.0f;
	float offset = 0.0f;
	float lacunarity = 0.0f;
	int octaves = 0;
	float gain = 0.0f;
	int noiseTypeIndex = 0;

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

#pragma once

#include "Node.h"

namespace ImGui {

static const char* NoiseTypeStr[] = {
	"double noise",
	"simplex noise",
	"ridged noise",
	"flow noise (rot. gradients)",
	"fbm",
	"fbm cascade",
	"fbm analytical derivatives",
	"flow noise fbm (time)",
	"ridged multi fractal (time)",
	"ridged multi fractal",
	"ridged multi fractal cascade",
	"iq noise",
	"analytical derivatives",
	"noise curl noise (time)",
	"worley noise",
	"worley noise fbm",
	"voronoi",
	"swissTurbulence",
	"jordanTurbulence",
	"poissonDiskDistribution"
};
static constexpr int numValues = IM_ARRAYSIZE(NoiseTypeStr);

class NoiseNode: public Node {
protected:
	float frequency = 0.0f;
	float offset = 0.0f;
	float lacunarity = 0.0f;
	int octaves = 0;
	float gain = 0.0f;
	int enumIndex = 0;

	static bool GetNoiseTypeFromEnumIndex(void*, int value, const char** pTxt) {
		if (!pTxt) {
			return false;
		}
		if (value >= 0 && value < numValues) {
			*pTxt = NoiseTypeStr[value];
		} else {
			*pTxt = "UNKNOWN";
		}
		return true;
	}

	const char* getTooltip() const override {
		return "NoiseNode tooltip.";
	}

	const char* getInfo() const override {
		return "NoiseNode info.\n\nThis is supposed to display some info about this node.";
	}

	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override {
		// [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
		defaultTitleTextColorOut = IM_COL32(220, 220, 220, 255);
		defaultTitleBgColorOut = IM_COL32(125, 35, 0, 255);
		defaultTitleBgColorGradientOut = -1.f;
	}

public:
	static NoiseNode* Create(const ImVec2& pos) {
		CREATE(NoiseNode);
		node->init("NoiseNode", pos, "position", "noise", int(NodeType::Noise));
		node->fields.addField(&node->frequency, 1, "Frequency", "Noise frequency", 2, 0, 1);
		node->fields.addField(&node->offset, 1, "Offset", "Noise offset", 2, 0, 1000);
		node->fields.addField(&node->lacunarity, 1, "Lacunarity", "Noise lacunarity", 2, 0, 10);
		node->fields.addField(&node->octaves, 1, "Octaves", "Noise octaves", 0, 1, 8);
		node->fields.addField(&node->gain, 1, "Gain", "Noise gain");
		node->fields.addFieldEnum(&node->enumIndex, numValues, &GetNoiseTypeFromEnumIndex, "Type", "Choose noise type");
		node->enumIndex = 1;
		return node;
	}
};

}

#pragma once

#include "Node.h"

namespace ImGui {

class NoiseNode: public Node {
protected:
	float Value[3];     // field 1
	ImVec4 Color;       // field 2
	int enumIndex = 0;      // field 3

	// Support static method for enumIndex (the signature is the same used by ImGui::Combo(...))
	static bool GetTextFromEnumIndex(void*, int value, const char** pTxt) {
		if (!pTxt) {
			return false;
		}
		static const char* values[] = { "APPLE", "LEMON", "ORANGE" };
		static int numValues = (int) (sizeof(values) / sizeof(values[0]));
		if (value >= 0 && value < numValues) {
			*pTxt = values[value];
		} else {
			*pTxt = "UNKNOWN";
		}
		return true;
	}

	virtual const char* getTooltip() const {
		return "NoiseNode tooltip.";
	}

	virtual const char* getInfo() const {
		return "NoiseNode info.\n\nThis is supposed to display some info about this node.";
	}

	virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
		// [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
		defaultTitleTextColorOut = IM_COL32(220, 220, 220, 255);
		defaultTitleBgColorOut = IM_COL32(125, 35, 0, 255);
		defaultTitleBgColorGradientOut = -1.f;
	}

public:
	static NoiseNode* Create(const ImVec2& pos) {
		CREATE(NoiseNode);
		node->init("NoiseNode", pos, "in1;in2;in3", "out1;out2", int(NodeType::Noise));
		node->fields.addField(&node->Value[0], 3, "Angles", "Three floats that are stored in radiant units internally", 2, 0, 360, nullptr, true);
		node->fields.addFieldColor(&node->Color.x, true, "Color", "color with alpha");
		node->fields.addFieldEnum(&node->enumIndex, 3, &GetTextFromEnumIndex, "Fruit", "Choose your favourite");
		node->Value[0] = 0;
		node->Value[1] = 3.14f;
		node->Value[2] = 4.68f;
		node->Color = ImColor(126, 200, 124, 230);
		node->enumIndex = 1;
		return node;
	}
};

}

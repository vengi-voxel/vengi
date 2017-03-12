#pragma once

#include "Node.h"

namespace ImGui {

class OutputNode: public Node {
protected:
	char imageName[128];

	const char* getTooltip() const override {
		return "Save noise as png.";
	}

	const char* getInfo() const override {
		return "OutputNode info.\n\nSave the noise input data as png.";
	}

	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override {
		// [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
		defaultTitleTextColorOut = IM_COL32(230, 180, 180, 255);
		defaultTitleBgColorOut = IM_COL32(40, 55, 55, 200);
		defaultTitleBgColorGradientOut = 0.025f;
	}
public:
	static OutputNode* Create(const ImVec2& pos) {
		CREATE(OutputNode);
		node->init("OutputNode", pos, "ch1;ch2;ch3;ch4", "", int(NodeType::Output));
		node->fields.addFieldTextEdit(&node->imageName[0], IM_ARRAYSIZE(node->imageName), "Image", "Image filename", ImGuiInputTextFlags_EnterReturnsTrue);
		return node;
	}
};

}

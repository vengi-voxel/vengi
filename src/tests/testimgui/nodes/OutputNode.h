#pragma once

#include "Node.h"

namespace ImGui {

class OutputNode: public Node {
protected:
	// No field values in this class

	virtual const char* getTooltip() const {
		return "OutputNode tooltip.";
	}

	virtual const char* getInfo() const {
		return "OutputNode info.\n\nThis is supposed to display some info about this node.";
	}

	virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
		// [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
		defaultTitleTextColorOut = IM_COL32(230, 180, 180, 255);
		defaultTitleBgColorOut = IM_COL32(40, 55, 55, 200);
		defaultTitleBgColorGradientOut = 0.025f;
	}

	virtual bool canBeCopied() const {
		return false;
	}

public:
	static OutputNode* Create(const ImVec2& pos) {
		CREATE(OutputNode);
		node->init("OutputNode", pos, "ch1;ch2;ch3;ch4", "", int(NodeType::Output));
		return node;
	}

protected:
	bool render(float /*nodeWidth*/) {
		ImGui::Text("There can be a single\ninstance of this class.\nTry and see if it's true!");
		return false;
	}
};

}

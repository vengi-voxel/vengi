#pragma once

#include "Node.h"

namespace ImGui {

class RGBANode: public Node {
protected:
	int imageWidth = 1024;
	int imageHeight = 1024;
	char imageName[128] = "";
	ImGui::NodeGraphEditor* nge = nullptr;

	const char* getTooltip() const override;

	const char* getInfo() const override;

	void onEdited() override;

	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;
public:
	static RGBANode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

}

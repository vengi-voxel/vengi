#pragma once

#include "NNode.h"
#include "video/Renderer.h"

namespace ImGui {

class RGBANode: public Node {
protected:
	int imageWidth = 512;
	int imageHeight = 512;
	char imageName[128] = "noise.png";
	ImGui::NodeGraphEditor* nge = nullptr;

	video::Id _texture = video::InvalidId;

	const char* getTooltip() const override;

	const char* getInfo() const override;

	void onEdited() override;

	bool render(float nodeWidth) override;

	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;
public:
	virtual ~RGBANode();
	static RGBANode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

}

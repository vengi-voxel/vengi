#pragma once

#include "NNode.h"
#include "video/Texture.h"

class RGBANode: public NodeBase {
protected:
	int imageWidth = 512;
	int imageHeight = 512;
	char imageName[128] = "noise.png";

	video::TexturePtr _texture;

	void onEdited() override;
	bool render(float nodeWidth) override;
	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;
public:
	RGBANode();
	virtual ~RGBANode();
	static RGBANode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

#pragma once

#include "NNode.h"
#include "core/Common.h"
#include "video/Texture.h"

class GraphNode: public NodeBase {
protected:
	static constexpr int BPP = 4;
	uint8_t *_graphBuffer = nullptr;
	int _graphHeight = 65;
	int _graphWidth = 350;
	int _offset = 0;

	video::TexturePtr _texture;

	void onEdited() override;
	bool render(float nodeWidth) override;
	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;
	int index(int x, int y) const;
public:
	GraphNode();
	virtual ~GraphNode();
	static GraphNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

inline int GraphNode::index(int x, int y) const {
	core_assert_msg(x >= 0, "x is smaller than 0: %i", x);
	core_assert_msg(x < _graphWidth * BPP, "x is out of bounds: %i", x);
	core_assert_msg(y >= 0, "y is smaller than 0: %i", y);
	return x * BPP + y * _graphWidth * BPP;
}

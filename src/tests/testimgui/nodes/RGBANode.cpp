#include "RGBANode.h"
#include "image/Image.h"

namespace ImGui {

const char* RGBANode::getTooltip() const {
	return "Save noise as png.";
}

const char* RGBANode::getInfo() const {
	return "RGBANode info.\n\nSave the noise input data as png.";
}

void RGBANode::onEdited() {
	Node* red = nge->getInputNodeForNodeAndSlot(this, 0);
	Node* green = nge->getInputNodeForNodeAndSlot(this, 1);
	Node* blue = nge->getInputNodeForNodeAndSlot(this, 2);
	Node* alpha = nge->getInputNodeForNodeAndSlot(this, 3);

}

void RGBANode::getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
	defaultTitleTextColorOut = IM_COL32(230, 180, 180, 255);
	defaultTitleBgColorOut = IM_COL32(40, 55, 55, 200);
	defaultTitleBgColorGradientOut = 0.025f;
}

RGBANode* RGBANode::Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
	CREATE(RGBANode);
	node->init("RGBANode", pos, "r;g;b;a", "", int(NodeType::RGBA));
	node->fields.addFieldTextEdit(&node->imageName[0], IM_ARRAYSIZE(node->imageName), "Image", "Image filename", ImGuiInputTextFlags_EnterReturnsTrue);
	node->nge = &nge;
	return node;
}

}

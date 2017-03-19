#include "ConstantNode.h"
#include <limits>

float ConstantNode::getNoise(int x, int y) {
	return constant;
}

ConstantNode* ConstantNode::Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
	ConstantNode* node = imguiAlloc<ConstantNode>();
	if (!node->setup(nge, pos, nullptr, "constant", NodeType::Constant)) {
		return nullptr;
	}
	node->fields.addField(&node->constant, 1, "Value", nullptr, 8, std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
	return node;
}

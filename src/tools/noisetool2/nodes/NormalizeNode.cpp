#include "NormalizeNode.h"
#include "noise/Noise.h"

float NormalizeNode::getNoise(int x, int y) {
	NNode* in1 = dynamic_cast<NNode*>(nge->getInputNodeForNodeAndSlot(this, 0));
	if (in1 != nullptr) {
		return noise::norm(in1->getNoise(x, y));
	}
	return 0.0f;
}

NormalizeNode* NormalizeNode::Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
	NormalizeNode* node = imguiAlloc<NormalizeNode>();
	if (!node->setup(nge, pos, "noise", "norm", NodeType::Normalize)) {
		return nullptr;
	}
	return node;
}

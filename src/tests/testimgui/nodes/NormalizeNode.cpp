#include "NormalizeNode.h"
#include "noise/Noise.h"

namespace ImGui {

const char* NormalizeNode::getTooltip() const {
	return "NormalizeNode tooltip.";
}

const char* NormalizeNode::getInfo() const {
	return "NormalizeNode info.\n\nNormalized the noise from [-1,1] to [0,1].";
}

float NormalizeNode::getNoise(int x, int y) {
	NNode* in1 = (NNode*)nge->getInputNodeForNodeAndSlot(this, 0);
	if (in1 != nullptr) {
		return noise::norm(in1->getNoise(x, y));
	}
	return 0.0f;
}

NormalizeNode* NormalizeNode::Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
	NormalizeNode* node = imguiAlloc<NormalizeNode>();
	node->init("NormalizeNode", pos, "in1", "out", int(NodeType::Normalize));
	node->nge = &nge;
	return node;
}

}

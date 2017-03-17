#include "NNode.h"

void NNode::onEdited() {
	markDirty();
}

void NNode::markDirty() {
	ImVector<ImGui::Node *> nodes;
	nge->getOutputNodesForNodeAndSlot(this, 0, nodes);
	for (ImGui::Node* node : nodes) {
		NNode* n = dynamic_cast<NNode*>(node);
		if (n == nullptr) {
			node->onEdited();
			continue;
		}
		n->markDirty();
	}
}

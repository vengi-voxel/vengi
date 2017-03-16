#pragma once

#include "Node.h"

namespace ImGui {

class CombineNode: public NNode {
protected:
	ImGui::NodeGraphEditor* nge = nullptr;

	const char* getTooltip() const override {
		return "CombineNode tooltip.";
	}

	const char* getInfo() const override {
		return "CombineNode info.\n\nThis is supposed to display some info about this node.";
	}

	float getNoise(int x, int y) override {
		NNode* in1 = (NNode*)nge->getInputNodeForNodeAndSlot(this, 0);
		NNode* in2 = (NNode*)nge->getInputNodeForNodeAndSlot(this, 1);
		if (in1 != nullptr && in2 != nullptr) {
			return in1->getNoise(x, y) + in2->getNoise(x, y);
		}
		if (in1 != nullptr) {
			return in1->getNoise(x, y);
		}
		if (in2 != nullptr) {
			return in2->getNoise(x, y);
		}
		return 0.0f;
	}

public:
	static CombineNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
		CombineNode* node = imguiAlloc<CombineNode>();
		node->init("CombineNode", pos, "in1;in2", "out", int(NodeType::Combine));
		node->nge = &nge;
		return node;
	}
};

}

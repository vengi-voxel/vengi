#pragma once

#include "Node.h"

namespace ImGui {

class NormalizeNode: public Node {
protected:
	const char* getTooltip() const override {
		return "NormalizeNode tooltip.";
	}

	const char* getInfo() const override {
		return "NormalizeNode info.\n\nNormalized the noise from [-1,1] to [0,1].";
	}
public:
	static NormalizeNode* Create(const ImVec2& pos) {
		CREATE(NormalizeNode);
		node->init("NormalizeNode", pos, "in1", "out", int(NodeType::Normalize));
		return node;
	}
};

}

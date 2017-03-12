#pragma once

#include "Node.h"

namespace ImGui {

class CombineNode: public Node {
protected:
	float fraction = 0.0f;

	virtual const char* getTooltip() const {
		return "CombineNode tooltip.";
	}

	virtual const char* getInfo() const {
		return "CombineNode info.\n\nThis is supposed to display some info about this node.";
	}
public:
	static CombineNode* Create(const ImVec2& pos) {
		CREATE(CombineNode);
		node->init("CombineNode", pos, "in1;in2", "out", int(NodeType::Combine));
		node->fields.addField(&node->fraction, 1, "Fraction", "Fraction of in1 that is mixed with in2", 2, 0, 1);
		node->fraction = 0.5f;
		return node;
	}
};

}

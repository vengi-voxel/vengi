#pragma once

#include "Node.h"

namespace ImGui {

class ColorNode: public Node {
protected:
	ImVec4 Color;       // field

	// Support static method for enumIndex (the signature is the same used by ImGui::Combo(...))
	static bool GetTextFromEnumIndex(void*, int value, const char** pTxt) {
		if (!pTxt) {
			return false;
		}
		static const char* values[] = { "APPLE", "LEMON", "ORANGE" };
		static int numValues = (int) (sizeof(values) / sizeof(values[0]));
		if (value >= 0 && value < numValues) {
			*pTxt = values[value];
		} else {
			*pTxt = "UNKNOWN";
		}
		return true;
	}

	virtual const char* getTooltip() const {
		return "ColorNode tooltip.";
	}

	virtual const char* getInfo() const {
		return "ColorNode info.\n\nThis is supposed to display some info about this node.";
	}

public:
	static ColorNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
		CREATE(ColorNode);
		node->init("ColorNode", pos, "", "r;g;b;a", int(NodeType::Color));
		node->fields.addFieldColor(&node->Color.x, true, "Color", "color with alpha");
		node->Color = ImColor(255, 255, 0, 255);
		return node;
	}
};

}

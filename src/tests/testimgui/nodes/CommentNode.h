#pragma once

#include "Node.h"

namespace ImGui {

class CommentNode: public Node {
protected:
	static const int TextBufferSize = 128;

	char comment[TextBufferSize];			    // field 1
	char comment2[TextBufferSize];			    // field 2
	char comment3[TextBufferSize];			    // field 3
	char comment4[TextBufferSize];			    // field 4
	bool flag = false;                          // field 5

	virtual const char* getTooltip() const {
		return "CommentNode tooltip.";
	}
	virtual const char* getInfo() const {
		return "CommentNode info.\n\nThis is supposed to display some info about this node.";
	}

public:
	static CommentNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
		CommentNode* node = imguiAlloc<CommentNode>();
		node->init("CommentNode", pos, "", "", int(NodeType::Comment));
		node->baseWidthOverride = 200.f;    // (optional) default base node width is 120.f;

		node->fields.addFieldTextEdit(&node->comment[0], TextBufferSize, "Single Line", "A single line editable field", ImGuiInputTextFlags_EnterReturnsTrue);
		node->fields.addFieldTextEditMultiline(&node->comment2[0], TextBufferSize, "Multi Line", "A multi line editable field", ImGuiInputTextFlags_AllowTabInput, 50);
		node->fields.addFieldTextEditMultiline(&node->comment3[0], TextBufferSize, "Multi Line 2", "A multi line read-only field", ImGuiInputTextFlags_ReadOnly, 50);
		node->fields.addFieldTextWrapped(&node->comment4[0], TextBufferSize, "Text Wrapped ReadOnly", "A text wrapped field");
		node->fields.addField(&node->flag, "Flag", "A boolean field");

		strcpy(node->comment, "Initial Text Line.");
		strcpy(node->comment2, "Initial Text Multiline.");
		static const char* tiger = "Tiger, tiger, burning bright\nIn the forests of the night,\nWhat immortal hand or eye\nCould frame thy fearful symmetry?";
		strncpy(node->comment3, tiger, TextBufferSize);
		static const char* txtWrapped = "I hope this text gets wrapped gracefully. But I'm not sure about it.";
		strncpy(node->comment4, txtWrapped, TextBufferSize);
		node->flag = true;

		return node;
	}
};

}

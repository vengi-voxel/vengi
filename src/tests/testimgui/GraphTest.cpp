#include "GraphTest.h"
#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui/imgui.h"
#include "imgui/addons/imguinodegrapheditor/imguinodegrapheditor.h"
#include "imgui/imgui_internal.h"

namespace ImGui {

enum class NodeType {
	Color,
	Combine,
	Comment,
	Noise,
	Output,

	Max
};
static const char* NodeTypeStr[] = {
	"Color",
	"Combine",
	"Comment",
	"Noise",
	"Output"
};
static_assert(int(NodeType::Max) == IM_ARRAYSIZE(NodeTypeStr), "Array size doesn't match enum values");

template<class T>
static T* allocate() {
	T* node = (T*) ImGui::MemAlloc(sizeof(T));
	return node;
}
#define CREATE(T) T* node = allocate<T>(); IM_PLACEMENT_NEW(node) T; T()

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
	static ColorNode* Create(const ImVec2& pos) {
		CREATE(ColorNode);
		node->init("ColorNode", pos, "", "r;g;b;a", int(NodeType::Color));
		node->fields.addFieldColor(&node->Color.x, true, "Color", "color with alpha");
		node->Color = ImColor(255, 255, 0, 255);
		return node;
	}
};

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
	static CommentNode* Create(const ImVec2& pos) {
		CREATE(CommentNode);
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

class NoiseNode: public Node {
protected:
	float Value[3];     // field 1
	ImVec4 Color;       // field 2
	int enumIndex = 0;      // field 3

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
		return "NoiseNode tooltip.";
	}

	virtual const char* getInfo() const {
		return "NoiseNode info.\n\nThis is supposed to display some info about this node.";
	}

	virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
		// [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
		defaultTitleTextColorOut = IM_COL32(220, 220, 220, 255);
		defaultTitleBgColorOut = IM_COL32(125, 35, 0, 255);
		defaultTitleBgColorGradientOut = -1.f;
	}

public:
	static NoiseNode* Create(const ImVec2& pos) {
		CREATE(NoiseNode);
		node->init("NoiseNode", pos, "in1;in2;in3", "out1;out2", int(NodeType::Noise));
		node->fields.addField(&node->Value[0], 3, "Angles", "Three floats that are stored in radiant units internally", 2, 0, 360, nullptr, true);
		node->fields.addFieldColor(&node->Color.x, true, "Color", "color with alpha");
		node->fields.addFieldEnum(&node->enumIndex, 3, &GetTextFromEnumIndex, "Fruit", "Choose your favourite");
		node->Value[0] = 0;
		node->Value[1] = 3.14f;
		node->Value[2] = 4.68f;
		node->Color = ImColor(126, 200, 124, 230);
		node->enumIndex = 1;
		return node;
	}
};

class OutputNode: public Node {
protected:
	// No field values in this class

	virtual const char* getTooltip() const {
		return "OutputNode tooltip.";
	}

	virtual const char* getInfo() const {
		return "OutputNode info.\n\nThis is supposed to display some info about this node.";
	}

	virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
		// [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
		defaultTitleTextColorOut = IM_COL32(230, 180, 180, 255);
		defaultTitleBgColorOut = IM_COL32(40, 55, 55, 200);
		defaultTitleBgColorGradientOut = 0.025f;
	}

	virtual bool canBeCopied() const {
		return false;
	}

public:
	static OutputNode* Create(const ImVec2& pos) {
		CREATE(OutputNode);
		node->init("OutputNode", pos, "ch1;ch2;ch3;ch4", "", int(NodeType::Output));
		return node;
	}

protected:
	bool render(float /*nodeWidth*/) {
		ImGui::Text("There can be a single\ninstance of this class.\nTry and see if it's true!");
		return false;
	}
};

static Node* MyNodeFactory(int nt, const ImVec2& pos) {
	switch ((NodeType)nt) {
	case NodeType::Color:
		return ColorNode::Create(pos);
	case NodeType::Combine:
		return CombineNode::Create(pos);
	case NodeType::Comment:
		return CommentNode::Create(pos);
	case NodeType::Noise:
		return NoiseNode::Create(pos);
	case NodeType::Output:
		return OutputNode::Create(pos);
	default:
		IM_ASSERT(true);
		return nullptr;
	}
	return nullptr;
}

static void linkCallback(const NodeLink& link, NodeGraphEditor::LinkState state, NodeGraphEditor& editor) {
}

void ShowExampleAppCustomNodeGraph() {
	static ImGui::NodeGraphEditor nge;
	if (nge.isInited()) {
		nge.registerNodeTypes(NodeTypeStr, int(NodeType::Max), MyNodeFactory, nullptr, -1);
		nge.registerNodeTypeMaxAllowedInstances(int(NodeType::Output), 1);
		nge.setLinkCallback(linkCallback);

		ImGui::Node* colorNode = nge.addNode(int(NodeType::Color), ImVec2(40, 50));
		ImGui::Node* complexNode = nge.addNode(int(NodeType::Noise), ImVec2(40, 150));
		ImGui::Node* combineNode = nge.addNode(int(NodeType::Combine), ImVec2(275, 80));
		ImGui::Node* outputNode = nge.addNode(int(NodeType::Output), ImVec2(520, 140));
		nge.addLink(colorNode, 0, combineNode, 0);
		nge.addLink(complexNode, 1, combineNode, 1);
		nge.addLink(complexNode, 0, outputNode, 1);
		nge.addLink(combineNode, 0, outputNode, 0);
		nge.show_style_editor = true;
		nge.show_load_save_buttons = true;
	}
	nge.render();
}

}

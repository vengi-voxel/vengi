#pragma once

#include "imgui/IMGUI.h"
#include "imgui/IMGUIInternal.h"
#include "imgui/IMGUIAddons.h"
#include "core/Log.h"

enum class NodeType {
	Add,
	Subtract,
	Multiply,
	Divide,
	Noise,
	RGBA,
	Normalize,
	Constant,

	Max
};
static const char* NodeTypeStr[] = {
	"Add",
	"Subtract",
	"Multiply",
	"Divide",
	"Noise",
	"Output",
	"Normalize",
	"Constant"
};
static_assert(int(NodeType::Max) == IM_ARRAYSIZE(NodeTypeStr), "Array size doesn't match enum values");

static const char* NodeTooltipStr[] = {
	"Adds up two input noise modules",
	"Subtracts two input noise modules",
	"Multiplies two input noise modules",
	"Divides two input noise modules",
	"Generate noise that can be used as input for other nodes",
	"Convert the noise input data into RGBA image",
	"Normalized the noise from [-1,1] to [0,1]",
	"Provide a constant as input parameter for other nodes"
};
static_assert(int(NodeType::Max) == IM_ARRAYSIZE(NodeTooltipStr), "Array size doesn't match enum values");

class NodeBase : public ImGui::Node {
protected:
	mutable std::string type;
public:
	ImGui::NodeGraphEditor* nge = nullptr;
	std::string info;
	const char* getInfo() const override;
	const char* getTooltip() const override;
	void setup(ImGui::NodeGraphEditor& nge, const ImVec2& pos, const char* inputSlots, const char* outputSlots, NodeType nodeTypeID);
};

/**
 * @brief A node that can deliver noise
 */
class NNode : public NodeBase {
public:
	virtual ~NNode() {}

	bool acceptsLink(Node* inputNode) override;
	void onEdited() override;

	virtual void markDirty();
	virtual float getNoise(int x, int y) = 0;
};

class ResultNode: public NNode {
public:
	float getNoise(int x, int y) override;
};

#define CREATE_RESULT_NODE(NodeTypeName) \
class NodeTypeName##Node: public ResultNode { \
protected: \
	bool canBeCopied() const override { return false; } \
public: \
	static NodeTypeName##Node* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) { \
		NodeTypeName##Node* node = imguiAlloc<NodeTypeName##Node>(); \
		node->setup(nge, pos, "val1;val1", "result", NodeType::NodeTypeName); \
		node->setOpen(false); \
		return node; \
	} \
}

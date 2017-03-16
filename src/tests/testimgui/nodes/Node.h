#pragma once

#include "imgui/IMGUI.h"
#include "imgui/IMGUIInternal.h"
#include "imgui/IMGUIAddons.h"

enum class NodeType {
	Color,
	Combine,
	Comment,
	Noise,
	RGBA,
	Normalize,

	Max
};
static const char* NodeTypeStr[] = {
	"Color",
	"Combine",
	"Comment",
	"Noise",
	"Output",
	"Normalize"
};
static_assert(int(NodeType::Max) == IM_ARRAYSIZE(NodeTypeStr), "Array size doesn't match enum values");

class NNode : public ImGui::Node {
public:
	virtual ~NNode() {}
	virtual float getNoise(int x, int y) = 0;
};

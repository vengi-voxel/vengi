#pragma once

#include "imgui/IMGUI.h"
#include "imgui/IMGUIInternal.h"
#include "imgui/IMGUIAddons.h"
#include "core/Log.h"

enum class NodeType {
	Combine,
	Noise,
	RGBA,
	Normalize,

	Max
};
static const char* NodeTypeStr[] = {
	"Combine",
	"Noise",
	"Output",
	"Normalize"
};
static_assert(int(NodeType::Max) == IM_ARRAYSIZE(NodeTypeStr), "Array size doesn't match enum values");

class NNode : public ImGui::Node {
public:
	ImGui::NodeGraphEditor* nge = nullptr;

	void onEdited() override;

	virtual void markDirty();
	virtual ~NNode() {}
	virtual float getNoise(int x, int y) = 0;
};

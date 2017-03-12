#pragma once

#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui/imgui.h"
#include "imgui/addons/imguinodegrapheditor/imguinodegrapheditor.h"
#include "imgui/imgui_internal.h"

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

template<class T>
static T* allocate() {
	T* node = (T*) ImGui::MemAlloc(sizeof(T));
	return node;
}
#define CREATE(T) T* node = allocate<T>(); IM_PLACEMENT_NEW(node) T; T()

class NNode : public ImGui::Node {
public:
	virtual ~NNode() {}
	virtual float getNoise(int x, int y) = 0;
};

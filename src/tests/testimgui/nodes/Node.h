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

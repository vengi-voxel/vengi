#pragma once

#include <dearimgui/imgui.h>
#define IMGUI_DEFINE_PLACEMENT_NEW
#include <dearimgui/imgui_internal.h>
#include <utility>

template<class T, class ... Args>
T* imguiAlloc(Args&&... args) {
	T* instance = (T*) ImGui::MemAlloc(sizeof(T));
	IM_PLACEMENT_NEW(instance) T(std::forward<Args>(args)...);
	return instance;
}

/**
 * @file
 * @ingroup UI
 */

#pragma once

#include "IMGUIInternal.h"
#include "core/Var.h"
#include <vector>
#include "core/collection/Array.h"
#include <glm/fwd.hpp>

namespace ImGui {

IMGUI_API bool InputVec3(const char* label, glm::ivec3& vec, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec3(const char* label, glm::vec3& vec, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarString(const char* label, core::VarPtr& var, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarFloat(const char* label, core::VarPtr& var, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool InputVarInt(const char* label, core::VarPtr& var, int step = 1, int step_fast = 100, ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool CheckboxVar(const char* label, core::VarPtr& var);
IMGUI_API bool CheckboxVar(const char* label, const char* varName);

template<class Collection>
static bool ComboStl(const char* label, int* current_item, const Collection& items, int height_in_items = -1) {
	return Combo(label, current_item,
		[](void* data, int idx, const char** out_text) {
			const Collection* vec = (const Collection*)data;
			if (idx < 0 || idx >= (int)vec->size()) {
				return false;
			}
			*out_text = (*vec)[idx].c_str();
			return true;
		},
		(void*) &items, (int)items.size(), height_in_items);
}
IMGUI_API void TooltipText(const char* text);



}

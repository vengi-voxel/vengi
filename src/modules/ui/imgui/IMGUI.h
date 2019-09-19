/**
 * @file
 * @ingroup UI
 */

#pragma once

#include "IMGUIInternal.h"
#include "core/Var.h"
#include <vector>

namespace ImGui {

IMGUI_API bool InputVarString(const char* label, core::VarPtr& var, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarFloat(const char* label, core::VarPtr& var, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool InputVarInt(const char* label, core::VarPtr& var, int step = 1, int step_fast = 100, ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool CheckboxVar(const char* label, core::VarPtr& var);
IMGUI_API bool CheckboxVar(const char* label, const char* varName);
IMGUI_API bool Combo(const char* label, int* current_item, const std::vector<std::string>& items, int height_in_items = -1);
IMGUI_API void TooltipText(const char* text);


}

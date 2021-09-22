/**
 * @file
 * @ingroup UI
 */

#pragma once

#include "IMGUIInternal.h"
#include "video/Types.h"
#include "core/Common.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include <glm/fwd.hpp>

namespace ImGui {

IMGUI_API void Image(video::Id handle, const glm::ivec2 &size, const glm::vec2 &uv0 = glm::vec2(0),
					 const glm::vec2 &uv1 = glm::vec2(1), const glm::vec4 &tintColor = glm::vec4(1),
					 const glm::vec4 &borderColor = glm::vec4(0));
IMGUI_API bool InputVec2(const char *label, glm::ivec2 &vec, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec2(const char* label, glm::vec2& vec, const char *format = "%.2f", ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec3(const char* label, glm::ivec3& vec, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVec3(const char* label, glm::vec3& vec, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarString(const char* label, const core::VarPtr& var, ImGuiInputTextFlags flags = 0);
IMGUI_API bool InputVarFloat(const char* label, const core::VarPtr& var, float step = 0.0f, float step_fast = 0.0f, ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool InputVarInt(const char* label, const core::VarPtr& var, int step = 1, int step_fast = 100, ImGuiInputTextFlags extra_flags = 0);
IMGUI_API bool CheckboxVar(const char* label, const core::VarPtr& var);
IMGUI_API bool CheckboxVar(const char* label, const char* varName);
IMGUI_API bool MenuItemCmd(const char *label, const char *command);

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
IMGUI_API bool TooltipText(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);
IMGUI_API void TextCentered(const char *text);
IMGUI_API bool ToggleButton(const char *text, bool state);
IMGUI_API void TableKeyValue(const char *key, CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(2);
IMGUI_API void TableKeyValue(const char *key, const core::String &value);
IMGUI_API bool DisabledButton(const char *text, bool disabled);
IMGUI_API float Size(float size);

IMGUI_API bool InputText(const char *label, core::String *str, ImGuiInputTextFlags flags = 0,
						 ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);
IMGUI_API bool InputTextMultiline(const char *label, core::String *str, const ImVec2 &size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0,
								  ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);
IMGUI_API bool InputTextWithHint(const char *label, const char *hint, core::String *str, ImGuiInputTextFlags flags = 0,
								 ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);
}

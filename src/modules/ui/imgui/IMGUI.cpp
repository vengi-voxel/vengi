/**
 * @file
 */

#include "IMGUI.h"
#include <SDL_stdinc.h>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ImGui {

bool InputVarString(const char* label, core::VarPtr& var, ImGuiInputTextFlags flags) {
	const core::String& buf = var->strVal();
	constexpr int size = 256;
	char newVal[size];
	SDL_snprintf(newVal, size, "%s", buf.c_str());
	flags &= ~ImGuiInputTextFlags_EnterReturnsTrue;
	if (InputText(label, newVal, size, flags)) {
		var->setVal(newVal);
		return true;
	}
	return false;
}

bool InputVarFloat(const char* label, core::VarPtr& var, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags extra_flags) {
	float v = var->floatVal();
	if (InputFloat(label, &v, step, step_fast, decimal_precision, extra_flags)) {
		var->setVal(v);
		return true;
	}
	return false;
}

bool InputVec3(const char* label, glm::vec3& vec, const char *format, ImGuiInputTextFlags flags) {
	return InputFloat3(label, glm::value_ptr(vec), format, flags);
}

bool InputVec3(const char* label, glm::ivec3& vec, ImGuiInputTextFlags flags) {
	return InputInt3(label, glm::value_ptr(vec), flags);
}

bool InputVarInt(const char* label, core::VarPtr& var, int step, int step_fast, ImGuiInputTextFlags extra_flags) {
	int v = var->intVal();
	if (InputInt(label, &v, step, step_fast, extra_flags)) {
		var->setVal(v);
		return true;
	}
	return false;
}

bool CheckboxVar(const char* label, core::VarPtr& var) {
	bool val = var->boolVal();
	if (Checkbox(label, &val)) {
		var->setVal(val);
		return true;
	}
	return false;
}

bool CheckboxVar(const char* label, const char* varName) {
	core::VarPtr var = core::Var::getSafe(varName);
	return CheckboxVar(label, var);
}

void TooltipText(const char* text) {
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text("%s", text);
		ImGui::EndTooltip();
	}
}

}

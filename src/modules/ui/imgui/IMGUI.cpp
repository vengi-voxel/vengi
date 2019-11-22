/**
 * @file
 */

#include "IMGUI.h"

namespace ImGui {

bool InputVarString(const char* label, core::VarPtr& var, ImGuiInputTextFlags flags) {
	const std::string& buf = var->strVal();
	constexpr int size = 256;
	char newVal[size];
	strncpy(newVal, buf.c_str(), size);
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

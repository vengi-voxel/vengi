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

bool Combo(const char* label, int* current_item, const std::vector<std::string>& items, int items_count, int height_in_items) {
	return Combo(label, current_item,
		[](void* data, int idx, const char** out_text) {
			const std::vector<std::string>* vec = (const std::vector<std::string>*)data;
			if (idx < 0 || idx >= (int)vec->size()) {
				return false;
			}
			*out_text = (*vec)[idx].c_str();
			return true;
		},
		(void*) &items, items_count, height_in_items);
}

void TooltipText(const char* text) {
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text("%s", text);
		ImGui::EndTooltip();
	}
}


}

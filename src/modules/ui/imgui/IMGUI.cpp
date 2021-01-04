/**
 * @file
 */

#include "IMGUI.h"
#include "core/Color.h"
#include "command/Command.h"
#include <SDL_stdinc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ImGui {

bool InputVarString(const char* label, const core::VarPtr& var, ImGuiInputTextFlags flags) {
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

bool InputVarFloat(const char* label, const core::VarPtr& var, float step, float step_fast, ImGuiInputTextFlags extra_flags) {
	float v = var->floatVal();
	if (InputFloat(label, &v, step, step_fast, "%.3f", extra_flags)) {
		var->setVal(v);
		return true;
	}
	return false;
}

bool InputVec2(const char* label, glm::ivec2& vec, ImGuiInputTextFlags flags) {
	return InputInt2(label, glm::value_ptr(vec), flags);
}

bool InputVec2(const char* label, glm::vec2& vec, const char *format, ImGuiInputTextFlags flags) {
	return InputFloat2(label, glm::value_ptr(vec), format, flags);
}

bool InputVec3(const char* label, glm::vec3& vec, const char *format, ImGuiInputTextFlags flags) {
	return InputFloat3(label, glm::value_ptr(vec), format, flags);
}

bool InputVec3(const char* label, glm::ivec3& vec, ImGuiInputTextFlags flags) {
	return InputInt3(label, glm::value_ptr(vec), flags);
}

bool InputVarInt(const char* label, const core::VarPtr& var, int step, int step_fast, ImGuiInputTextFlags extra_flags) {
	int v = var->intVal();
	if (InputInt(label, &v, step, step_fast, extra_flags)) {
		var->setVal(v);
		return true;
	}
	return false;
}

bool CheckboxVar(const char* label, const core::VarPtr& var) {
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

bool TooltipText(const char* msg, ...) {
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();

		va_list ap;
		constexpr size_t bufSize = 4096;
		char text[bufSize];

		va_start(ap, msg);
		SDL_vsnprintf(text, bufSize, msg, ap);
		text[sizeof(text) - 1] = '\0';
		va_end(ap);

		ImGui::Text("%s", text);
		ImGui::EndTooltip();
		return true;
	}
	return false;
}

void TextCentered(const char *text) {
	const float w = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - w) * 0.5f);
	ImGui::TextUnformatted(text);
}

void Image(video::Id handle, const glm::ivec2& size, const glm::vec2& uv0, const glm::vec2& uv1) {
	ImGui::Image((void*)(intptr_t)handle, ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
}

bool MenuItemCmd(const char *label, const char *command) {
	if (ImGui::MenuItem(label)) {
		command::Command::execute("%s", command);
		return true;
	}
	return false;
}

void TableKeyValue(const char *key, const char *msg, ...) {
	ImGui::TableNextRow();
	ImGui::TextUnformatted(key);
	ImGui::TableNextColumn();
	va_list ap;
	va_start(ap, msg);
	ImGui::TextV(msg, ap);
	va_end(ap);
}

void TableKeyValue(const char *key, const core::String &value) {
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(key);
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(value.c_str());
}

bool ToggleButton(const char *text, bool state) {
	if (state) {
		const ImVec4& buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		const ImVec4& buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImGui::PushStyleColor(ImGuiCol_Button, core::Color::brighter(buttonColor));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, core::Color::brighter(buttonHoveredColor));
	}
	const bool pressed = ImGui::Button(text);
	if (state) {
		ImGui::PopStyleColor(2);
	}
	return pressed;
}

bool DisabledButton(const char *text, bool disabled) {
	if (disabled) {
		const ImVec4& buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		const ImVec4& buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImGui::PushStyleColor(ImGuiCol_Button, core::Color::gray(buttonColor));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, core::Color::gray(buttonHoveredColor));
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}
	const bool pressed = ImGui::Button(text);
	if (disabled) {
		ImGui::PopItemFlag();
		ImGui::PopStyleColor(2);
	}
	return pressed;
}

int Size(int size) {
	return (int)((float)size * ImGui::GetWindowDpiScale());
}

}

/**
 * @file
 */

#include "IMGUIEx.h"
#include "IMGUIApp.h"
#include "ScopedStyle.h"
#include "command/CommandHandler.h"
#include "core/Color.h"
#include "command/Command.h"
#include "imgui.h"
#include "video/WindowedApp.h"
#include <SDL_stdinc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "dearimgui/imgui_internal.h"

namespace ImGui {

namespace _priv {

struct InputTextCallback_UserData {
	core::String *Str;
	ImGuiInputTextCallback ChainCallback;
	void *ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData *data) {
	InputTextCallback_UserData *userData = (InputTextCallback_UserData *)data->UserData;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		// Resize string callback
		// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back
		// to what we want.
		core::String *str = userData->Str;
		const int lold = str->size();
		core_assert(data->Buf == str->c_str());
		str->reserve(data->BufTextLen);
		for (int i = lold; i < data->BufTextLen; ++i) {
			str->append(" ");
		}
		data->Buf = (char *)str->c_str();
	} else if (userData->ChainCallback) {
		// Forward to user callback, if any
		data->UserData = userData->ChainCallbackUserData;
		return userData->ChainCallback(data);
	}
	return 0;
}

}

bool InputText(const char *label, core::String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *userData) {
	core_assert((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	_priv::InputTextCallback_UserData cb_userData;
	cb_userData.Str = str;
	cb_userData.ChainCallback = callback;
	cb_userData.ChainCallbackUserData = userData;
	return InputText(label, str->c_str(), str->capacity(), flags, _priv::InputTextCallback, &cb_userData);
}

bool InputTextMultiline(const char *label, core::String *str, const ImVec2 &size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *userData) {
	core_assert((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	_priv::InputTextCallback_UserData cb_userData;
	cb_userData.Str = str;
	cb_userData.ChainCallback = callback;
	cb_userData.ChainCallbackUserData = userData;
	return InputTextMultiline(label, str->c_str(), str->capacity(), size, flags, _priv::InputTextCallback, &cb_userData);
}

bool InputTextWithHint(const char *label, const char *hint, core::String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *userData) {
	core_assert((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	_priv::InputTextCallback_UserData cb_userData;
	cb_userData.Str = str;
	cb_userData.ChainCallback = callback;
	cb_userData.ChainCallbackUserData = userData;
	return InputTextWithHint(label, hint, str->c_str(), str->capacity(), flags, _priv::InputTextCallback, &cb_userData);
}

bool InputVarString(const char* label, const core::VarPtr& var, ImGuiInputTextFlags flags) {
	core::String buf = var->strVal();
	flags &= ~ImGuiInputTextFlags_EnterReturnsTrue;
	if (InputText(label, &buf, flags)) {
		if (var->setVal(buf)) {
			return true;
		}
	}
	if (var->help()) {
		TooltipText("%s", var->help());
	}
	return false;
}

bool InputVarFloat(const char* label, const core::VarPtr& var, float step, float step_fast, ImGuiInputTextFlags extra_flags) {
	float v = var->floatVal();
	if (InputFloat(label, &v, step, step_fast, "%.3f", extra_flags)) {
		if (var->setVal(v)) {
			return true;
		}
	}
	if (var->help()) {
		TooltipText("%s", var->help());
	}
	return false;
}

bool InputVarFloat(const char* label, const char *varName, float step, float step_fast, ImGuiInputTextFlags extra_flags) {
	core::VarPtr var = core::Var::getSafe(varName);
	return InputVarFloat(label, var, step, step_fast, extra_flags);
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
		if (var->setVal(v)) {
			return true;
		}
	}
	if (var->help()) {
		TooltipText("%s", var->help());
	}
	return false;
}

bool InputVarInt(const char* label, const char* varName, int step, int step_fast, ImGuiInputTextFlags extra_flags) {
	core::VarPtr var = core::Var::getSafe(varName);
	return InputVarInt(label, var, step, step_fast, extra_flags);
}

bool CheckboxVar(const char* label, const core::VarPtr& var) {
	bool val = var->boolVal();
	if (Checkbox(label, &val)) {
		if (var->setVal(val)) {
			return true;
		}
	}
	if (var->help()) {
		TooltipText("%s", var->help());
	}
	return false;
}

bool CheckboxVar(const char* label, const char* varName) {
	core::VarPtr var = core::Var::getSafe(varName);
	if (CheckboxVar(label, var)) {
		return true;
	}
	return false;
}

bool SliderVarInt(const char* label, const core::VarPtr& var, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
	int val = var->intVal();
	if (SliderInt(label, &val, v_min, v_max, format, flags)) {
		var->setVal(val);
		return true;
	}
	return false;
}

bool SliderVarInt(const char* label, const char* varName, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
	core::VarPtr var = core::Var::getSafe(varName);
	return SliderVarInt(label, var, v_min, v_max, format, flags);
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
	const ImVec2& size = ImGui::CalcTextSize(text);
	const ImVec2& maxs = ImGui::GetWindowContentRegionMax();
	const ImVec2 restore = ImGui::GetCursorPos();
	ImGui::SetCursorPosX((maxs.x - size.x) * 0.5f);
	ImGui::SetCursorPosY((maxs.y - size.y) * 0.5f);
	ImGui::TextUnformatted(text);
	ImGui::SetCursorPos(restore);
}

void Headline(const char *text) {
	ui::imgui::ScopedStyle font;
	font.setFont(imguiApp()->bigFont());
	ImGui::Text("%s", text);
}

void Image(video::Id handle, const glm::ivec2 &size, const glm::vec2 &uv0, const glm::vec2 &uv1, const glm::vec4 &tintColor, const glm::vec4 &borderColor) {
	ImGui::Image((ImTextureID)(intptr_t)handle, ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y), ImVec4(tintColor), ImVec4(borderColor));
}

void Image(video::Id handle, const ImVec2 &size, const ImVec2 &uv0, const ImVec2 &uv1, const ImVec4 &tintColor, const ImVec4 &borderColor) {
	ImGui::Image((ImTextureID)(intptr_t)handle, size, uv0, uv1, tintColor, borderColor);
}

bool ImageButton(video::Id handle, const ImVec2 &size, const ImVec2 &uv0, const ImVec2 &uv1, int frame_padding, const ImVec4 &borderColor, const ImVec4 &tintColor) {
	if (frame_padding >= 0)
		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2((float)frame_padding, (float)frame_padding));
	PushID((void *)(intptr_t)handle);
	ImGuiWindow* window = GetCurrentWindow();
	const ImGuiID id = window->GetID("#image");
	PopID();
	bool state = ImGui::ImageButtonEx(id, (ImTextureID)(intptr_t)handle, size, uv0, uv1, borderColor, tintColor);
	if (frame_padding >= 0)
		PopStyleVar();
	return state;
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

void TooltipCommand(const char *command) {
	if (ImGui::IsItemHovered()) {
		char buf[64];
		SDL_strlcpy(buf, command, sizeof(buf));
		buf[sizeof(buf) - 1] = '\0';
		char *firstwhitespace = SDL_strchr(buf, ' ');
		if (firstwhitespace) {
			*firstwhitespace = '\0';
		}
		const char *help = command::help(buf);
		if (help && *help) {
			ImGui::TooltipText("%s", help);
		}
	}
}

const char *CommandButton(const char *title, const char *command, const char *tooltip, float width, command::CommandExecutionListener* listener) {
	if (ImGui::Button(title, ImVec2(width, 0))) {
		if (command::executeCommands(command, listener) > 0) {
			return command;
		}
	}
	if (tooltip != nullptr) {
		ImGui::TooltipText("%s", tooltip);
	} else {
		TooltipCommand(command);
	}
	return nullptr;
}

bool URLButton(const char *title, const char *url) {
	video::WindowedApp* app = video::WindowedApp::getInstance();
	const core::String& cmd = core::String::format("url %s", url);
	if (CommandButton(title, cmd.c_str())) {
		app->minimize();
		return true;
	}
	return false;
}

const char *CommandMenuItem(const char *title, const char *command, bool enabled, command::CommandExecutionListener* listener) {
	video::WindowedApp* app = video::WindowedApp::getInstance();
	const core::String& keybinding = app->getKeyBindingsString(command);
	if (ImGui::MenuItem(title, keybinding.c_str(), false, enabled)) {
		if (command::executeCommands(command, listener) > 0) {
			return command;
		}
	}
	TooltipCommand(command);
	return nullptr;
}

void URLItem(const char *title, const char *url) {
	video::WindowedApp* app = video::WindowedApp::getInstance();
	const core::String& cmd = core::String::format("url %s", url);
	if (CommandButton(title, cmd.c_str())) {
		app->minimize();
	}
}

bool Fullscreen(const char *title, ImGuiWindowFlags additionalFlags) {
	ui::imgui::IMGUIApp* app = (ui::imgui::IMGUIApp*)video::WindowedApp::getInstance();
	SetNextWindowSize(app->frameBufferDimension());
	SetNextWindowPos(ImVec2(0.0f, 0.0f));
	return Begin(title, nullptr,
				 additionalFlags | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration |
					 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
					 ImGuiWindowFlags_NoDocking);
}

// https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
void LoadingIndicatorCircle(const char *label, const float indicator_radius, const ImVec4 &main_color,
							const ImVec4 &backdrop_color, const int circle_count, const float speed) {
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems) {
		return;
	}

	const ImVec2& maxs = ImGui::GetWindowContentRegionMax();
	const ImVec2 restore = ImGui::GetCursorPos();
	ImGui::SetCursorPosX(maxs.x / 2.0f - indicator_radius);
	ImGui::SetCursorPosY(maxs.y / 2.0f - indicator_radius);

	ImGuiContext &g = *GImGui;
	const ImGuiID id = window->GetID(label);

	{
		ui::imgui::ScopedStyle style;
		ui::imgui::IMGUIApp *app = imguiApp();
		style.setFont(app->bigFont());
		ImGui::TextCentered(label);
	}

	const ImVec2 pos = window->DC.CursorPos;
	const float circle_radius = indicator_radius / 10.0f;
	const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
	ItemSize(bb, g.Style.FramePadding.y);
	if (!ItemAdd(bb, id)) {
		ImGui::SetCursorPos(restore);
		return;
	}
	const float t = (float)g.Time;
	const float degree_offset = 2.0f * glm::pi<float>() / (float)circle_count;
	for (int i = 0; i < circle_count; ++i) {
		const float x = indicator_radius * glm::sin(degree_offset * (float)i);
		const float y = indicator_radius * glm::cos(degree_offset * (float)i);
		const float growth = core_max(0.0f, glm::sin(t * speed - i * degree_offset));
		ImVec4 color;
		color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
		color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
		color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
		color.w = 1.0f;
		window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y),
										  circle_radius + growth * circle_radius, GetColorU32(color));
	}
	ImGui::SetCursorPos(restore);
}

}

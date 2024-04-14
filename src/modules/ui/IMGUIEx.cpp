/**
 * @file
 */

#include "IMGUIEx.h"
#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "ScopedStyle.h"
#include "command/CommandHandler.h"
#include "core/Color.h"
#include "command/Command.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "video/Camera.h"
#include "video/FileDialogOptions.h"
#include "video/WindowedApp.h"
#include <SDL_stdinc.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

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

bool InputFile(const char *label, core::String *file, const io::FormatDescription *descriptions, ImGuiInputTextFlags flags) {
	const bool v = InputText(label, file, flags);
	SameLine();
	if (Button(ICON_LC_FILE)) {
		video::FileDialogOptions options;
		imguiApp()->openDialog([file] (const core::String &filename, const io::FormatDescription *desc) {
			*file = filename;
		}, options, descriptions);
	}
	return v;
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

bool IconCheckboxVar(const char *icon, const char *label, const core::VarPtr &var) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return CheckboxVar(labelWithIcon.c_str(), var);
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

bool IconCheckboxVar(const char *icon, const char* label, const char* varName) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return CheckboxVar(labelWithIcon.c_str(), varName);
}

bool CheckboxVar(const char* label, const char* varName) {
	core::VarPtr var = core::Var::getSafe(varName);
	if (CheckboxVar(label, var)) {
		return true;
	}
	return false;
}

bool IconCheckboxFlags(const char *icon, const char *label, int *flags, int flags_value) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return CheckboxFlags(labelWithIcon.c_str(), flags, flags_value);
}

bool IconCollapsingHeader(const char *icon, const char *label, ImGuiTreeNodeFlags flags) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return CollapsingHeader(labelWithIcon.c_str(), flags);
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

bool SliderVarFloat(const char* label, const core::VarPtr& var, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
	float val = var->floatVal();
	if (SliderFloat(label, &val, v_min, v_max, format, flags)) {
		var->setVal(val);
		return true;
	}
	return false;
}

bool SliderVarFloat(const char* label, const char* varName, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
	core::VarPtr var = core::Var::getSafe(varName);
	return SliderVarFloat(label, var, v_min, v_max, format, flags);
}

bool ColorEdit3Var(const char* label, const char* varName) {
	glm::vec3 col;
	core::Var::getSafe(varName)->vec3Val(&col[0]);
	if (ImGui::ColorEdit3(label, glm::value_ptr(col))) {
		const core::String &c = core::string::format("%f %f %f", col.x, col.y, col.z);
		core::Var::getSafe(varName)->setVal(c);
		return true;
	}
	return false;
}

float CalcTextWidth(const char *label, bool withPadding) {
	const float w = ImGui::CalcTextSize(label).x;
	if (!withPadding) {
		return w;
	}
	return w + ImGui::GetStyle().FramePadding.x * 2.0f;
}

float CalcComboWidth(const char *previewLabel, bool withPadding) {
	return CalcTextWidth(previewLabel, withPadding) + ImGui::GetFrameHeightWithSpacing();
}

void TextWrappedUnformatted(const char* text) {
    ImGuiContext& g = *GImGui;
    const bool need_backup = (g.CurrentWindow->DC.TextWrapPos < 0.0f);  // Keep existing wrap position if one is already set
    if (need_backup)
        PushTextWrapPos(0.0f);
    ImGuiWindow* window = GetCurrentWindow();
    if (!window->SkipItems)
	    TextEx(text, nullptr, ImGuiTextFlags_NoWidthForLargeClippedText);
    if (need_backup)
        PopTextWrapPos();
}

bool TooltipTextUnformatted(const char *text) {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(text);
		ImGui::EndTooltip();
		return true;
	}
	return false;}

bool TooltipText(const char* msg, ...) {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::BeginTooltip();

		va_list ap;
		constexpr size_t bufSize = 4096;
		char text[bufSize];

		va_start(ap, msg);
		SDL_vsnprintf(text, bufSize, msg, ap);
		text[sizeof(text) - 1] = '\0';
		va_end(ap);

		ImGui::TextUnformatted(text);
		ImGui::EndTooltip();
		return true;
	}
	return false;
}

void TextCentered(const char *label, bool reset) {
	const ImVec2& size = ImGui::CalcTextSize(label);
	const ImVec2& maxs = ImGui::GetWindowContentRegionMax();
	const ImVec2 restore = ImGui::GetCursorPos();
	ImGui::SetCursorPosX((maxs.x - size.x) * 0.5f);
	ImGui::SetCursorPosY((maxs.y - size.y) * 0.5f);
	ImGui::TextUnformatted(label);
	if (reset) {
		ImGui::SetCursorPos(restore);
	}
}

void Headline(const char *label) {
	ui::ScopedStyle font;
	font.setFont(imguiApp()->bigFont());
	ImGui::TextUnformatted(label);
}

void Image(video::Id handle, const glm::ivec2 &size, const glm::vec2 &uv0, const glm::vec2 &uv1, const glm::vec4 &tintColor, const glm::vec4 &borderColor) {
	ImGui::Image((ImTextureID)(intptr_t)handle, ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y), ImVec4(tintColor), ImVec4(borderColor));
}

void Image(video::Id handle, const ImVec2 &size, const ImVec2 &uv0, const ImVec2 &uv1, const ImVec4 &tintColor, const ImVec4 &borderColor) {
	ImGui::Image((ImTextureID)(intptr_t)handle, size, uv0, uv1, tintColor, borderColor);
}

bool ImageButton(video::Id handle, const ImVec2 &size, const ImVec2 &uv0, const ImVec2 &uv1, int frame_padding, const ImVec4 &borderColor, const ImVec4 &tintColor, ImGuiButtonFlags flags) {
	if (frame_padding >= 0)
		PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2((float)frame_padding, (float)frame_padding));
	PushID((void *)(intptr_t)handle);
	ImGuiWindow* window = GetCurrentWindow();
	const ImGuiID id = window->GetID("#image");
	PopID();
	bool state = ImGui::ImageButtonEx(id, (ImTextureID)(intptr_t)handle, size, uv0, uv1, borderColor, tintColor, flags);
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

bool ToggleButton(const char *label, bool state) {
	if (state) {
		const ImVec4& buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		const ImVec4& buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImGui::PushStyleColor(ImGuiCol_Button, core::Color::brighter(buttonColor));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, core::Color::brighter(buttonHoveredColor));
	}
	const bool pressed = ImGui::Button(label);
	if (state) {
		ImGui::PopStyleColor(2);
	}
	return pressed;
}

bool DisabledButton(const char *label, bool disabled, const ImVec2& size) {
	if (disabled) {
		const ImVec4& buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		const ImVec4& buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImGui::PushStyleColor(ImGuiCol_Button, core::Color::gray(buttonColor));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, core::Color::gray(buttonHoveredColor));
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}
	const bool pressed = ImGui::Button(label, size);
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
		const core::String &help = command::help(buf);
		if (!help.empty()) {
			ImGui::TooltipText("%s", help.c_str());
		}
	}
}

const char *CommandButton(const char *label, const char *command, const char *tooltip, const ImVec2 &size, command::CommandExecutionListener* listener) {
	if (ImGui::Button(label, size)) {
		if (command::executeCommands(command, listener) > 0) {
			return command;
		}
	}
	ui::ScopedStyle style;
	style.setFont(imguiApp()->defaultFont());
	if (tooltip != nullptr) {
		ImGui::TooltipText("%s", tooltip);
	} else {
		TooltipCommand(command);
	}
	return nullptr;
}

const char *CommandIconButton(const char *icon, const char *label, const char *command, command::CommandExecutionListener &listener) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return CommandButton(labelWithIcon.c_str(), command, listener);
}

const char *CommandButton(const char *label, const char *command, command::CommandExecutionListener& listener) {
	return CommandButton(label, command, nullptr, {0.0f, 0.0f}, &listener);
}

bool CommandRadioButton(const char *label, const core::String &command, bool enabled, command::CommandExecutionListener* listener) {
	const bool activated = ImGui::RadioButton(label, enabled);
	if (activated) {
		command::executeCommands(command, listener);
	}
	ImGui::TooltipCommand(command.c_str());
	return activated;
}

const char *CommandIconMenuItem(const char *icon, const char *label, const char *command, bool enabled, command::CommandExecutionListener* listener) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return CommandMenuItem(labelWithIcon.c_str(), command, enabled, listener);
}

const char *CommandMenuItem(const char *label, const char *command, bool enabled, command::CommandExecutionListener* listener) {
	const core::String& keybinding = imguiApp()->getKeyBindingsString(command);
	if (ImGui::MenuItem(label, keybinding.c_str(), false, enabled)) {
		if (command::executeCommands(command, listener) > 0) {
			return command;
		}
	}
	TooltipCommand(command);
	return nullptr;
}

static inline void AddUnderLine(ImColor color) {
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	min.y = max.y;
	ImGui::GetWindowDrawList()->AddLine(min, max, color, 1.0f);
}

bool IconSelectable(const char *icon, const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return Selectable(labelWithIcon.c_str(), selected, flags, size);
}

bool URLIconButton(const char *icon, const char *label, const char *url) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return URLButton(labelWithIcon.c_str(), url);
}

bool URLButton(const char *label, const char *url) {
	const core::String& cmd = core::String::format("url \"%s\"", url);
	if (CommandButton(label, cmd.c_str())) {
		imguiApp()->minimize();
		return true;
	}
	return false;
}

void URLIconItem(const char *icon, const char *label, const char *url, float width) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	URLItem(labelWithIcon.c_str(), url, width);
}

// https://gist.github.com/dougbinks/ef0962ef6ebe2cadae76c4e9f0586c69#file-imguiutils-h-L219
void URLItem(const char *label, const char *url, float width) {
	ImGui::TextUnformatted(label);
	if (ImGui::IsItemHovered()) {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			const core::String &cmd = core::String::format("url \"%s\"", url);
			command::executeCommands(cmd);
		}
		AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
		ImGui::SetTooltip(_("Open in browser\n%s"), url);
	}
}
bool ButtonFullWidth(const char *label) {
	return Button(label, ImVec2(ImGui::GetContentRegionAvail().x, 0));
}

bool IconTreeNodeEx(const char *icon, const char *label, ImGuiTreeNodeFlags flags) {
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	core::String labelWithIcon = core::string::format("%s %s", icon, label);
	return TreeNodeBehavior(window->GetID(label), flags, labelWithIcon.c_str(), nullptr);
}

bool Fullscreen(const char *label, ImGuiWindowFlags additionalFlags) {
	SetNextWindowSize(imguiApp()->frameBufferDimension());
	SetNextWindowPos(ImVec2(0.0f, 0.0f));
	return Begin(label, nullptr,
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
	ImGui::SetCursorPosX(maxs.x / 2.0f - indicator_radius);
	ImGui::SetCursorPosY(maxs.y / 2.0f - indicator_radius);

	ImGuiContext &g = *GImGui;
	const ImGuiID id = window->GetID(label);

	{
		ui::ScopedStyle style;
		style.setFont(imguiApp()->bigFont());
		ImGui::TextCentered(label, true);
	}

	const ImVec2 pos = window->DC.CursorPos;
	const float circle_radius = indicator_radius / 10.0f;
	const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
	ItemSize(bb, g.Style.FramePadding.y);
	if (!ItemAdd(bb, id)) {
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
}

// ImGuizmo
void DrawGrid(ImDrawList *drawList, const video::Camera &camera, const glm::mat4 &matrix, const float gridSize) {
	glm::mat4 res = matrix * camera.viewProjectionMatrix();

	for (float f = -gridSize; f <= gridSize; f += 1.0f) {
		for (int dir = 0; dir < 2; dir++) {
			glm::vec3 ptA(dir ? -gridSize : f, 0.0f, dir ? f : -gridSize);
			glm::vec3 ptB(dir ? gridSize : f, 0.0f, dir ? f : gridSize);
			bool visible = true;
			for (int i = 0; i < math::FRUSTUM_PLANES_MAX; i++) {
				const math::Plane &plane = camera.frustum().plane((math::FrustumPlanes)i);
				const float dA = plane.distanceToPlane(ptA);
				const float dB = plane.distanceToPlane(ptB);
				if (dA < 0.0f && dB < 0.0f) {
					visible = false;
					break;
				}
				if (dA > 0.0f && dB > 0.0f) {
					continue;
				}
				if (dA < 0.0f) {
					const float len = fabsf(dA - dB);
					const float t = fabsf(dA) / len;
					ptA = glm::mix(ptA, ptB, t);
				}
				if (dB < 0.0f) {
					const float len = fabsf(dB - dA);
					const float t = fabsf(dB) / len;
					ptB = glm::mix(ptB, ptA, t);
				}
			}
			if (visible) {
				uint32_t col = IM_COL32(0x80, 0x80, 0x80, 0xFF);
				col = (glm::mod(glm::abs(f), 10.f) < FLT_EPSILON) ? IM_COL32(0x90, 0x90, 0x90, 0xFF) : col;
				col = (glm::abs(f) < FLT_EPSILON) ? IM_COL32(0x40, 0x40, 0x40, 0xFF) : col;

				float thickness = 1.0f;
				thickness = (glm::mod(glm::abs(f), 10.0f) < FLT_EPSILON) ? 1.5f : thickness;
				thickness = (glm::abs(f) < FLT_EPSILON) ? 2.3f : thickness;

				const glm::ivec2 wA = camera.worldToScreen(res, ptA);
				const glm::ivec2 wB = camera.worldToScreen(res, ptB);
				drawList->AddLine(wA, wB, col, thickness);
			}
		}
	}
}

void IconDialog(const char *icon, const char *label) {
	ImGui::AlignTextToFramePadding();
	ImGui::PushFont(imguiApp()->bigFont());
	ImGui::TextUnformatted(icon);
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	ImGui::TextWrapped("%s", label);
	ImGui::Spacing();
	ImGui::Separator();
}

bool IconCheckbox(const char *icon, const char *label, bool *v) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return ImGui::Checkbox(labelWithIcon.c_str(), v);
}

bool BeginIconCombo(const char *icon, const char* label, const char* preview_value, ImGuiComboFlags flags) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return ImGui::BeginCombo(labelWithIcon.c_str(), preview_value, flags);
}

bool BeginIconMenu(const char *icon, const char *label, bool enabled) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return BeginMenu(labelWithIcon.c_str(), enabled);
}

bool IconMenuItem(const char *icon, const char* label, const char* shortcut, bool selected, bool enabled) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return MenuItem(labelWithIcon.c_str(), shortcut, selected, enabled);
}

bool IconButton(const char *icon, const char *label, const ImVec2 &size) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return Button(labelWithIcon.c_str(), size);
}

bool DisabledIconButton(const char *icon, const char *label, bool disabled, const ImVec2 &size) {
	core::String labelWithIcon = core::string::format("%s %s###%s", icon, label, label);
	return DisabledButton(labelWithIcon.c_str(), disabled, size);
}

}

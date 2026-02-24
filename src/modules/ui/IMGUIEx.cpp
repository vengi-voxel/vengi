/**
 * @file
 */

#include "IMGUIEx.h"
#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "ScopedID.h"
#include "ScopedStyle.h"
#include "Style.h"
#include "app/App.h"
#include "color/ColorUtil.h"
#include "core/StringUtil.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "dearimgui/imgui_internal.h"
#include "io/FormatDescription.h"
#include "math/Axis.h"
#include "video/FileDialogOptions.h"
#include "video/WindowedApp.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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
		core_assert(data->Buf == str->c_str());
		str->resize(data->BufTextLen);
		data->Buf = (char *)str->c_str();
	} else if (userData->ChainCallback) {
		// Forward to user callback, if any
		data->UserData = userData->ChainCallbackUserData;
		return userData->ChainCallback(data);
	}
	return 0;
}

static core::String getId(const char *icon, const char *label) {
	core::String id(icon);
	id += ' ';
	id += label;
	id += "###";
	id += label;
	return id;
}

static void AxisStyleButton(ui::ScopedStyle &style, math::Axis axis) {
	switch (axis) {
	case math::Axis::X: {
		const glm::vec4 &c = style::color(style::ColorAxisX);
		style.setColor(ImGuiCol_Text, color::contrastTextColor(c));
		style.setButtonColor(c);
		break;
	}
	case math::Axis::Y: {
		const glm::vec4 &c = style::color(style::ColorAxisY);
		style.setColor(ImGuiCol_Text, color::contrastTextColor(c));
		style.setButtonColor(c);
		break;
	}
	case math::Axis::Z: {
		const glm::vec4 &c = style::color(style::ColorAxisZ);
		style.setColor(ImGuiCol_Text, color::contrastTextColor(c));
		style.setButtonColor(c);
		break;
	}
	default:
		break;
	}
}

template<typename ValueType>
static bool InputXYZImpl(const char *label, glm::vec<3, ValueType> &vec, const char *format, ImGuiInputTextFlags flags, ValueType step, ValueType step_fast) {
	constexpr bool isFloat = std::is_same<ValueType, float>::value;
	constexpr bool isInt = std::is_same<ValueType, int>::value;
	static_assert(isFloat || isInt, "VecType must be glm::vec3 or glm::ivec3");

	if (ImGui::GetCurrentTable()) {
		ImGui::BeginGroup();
		ImGui::TableNextColumn();
		const float h = ImGui::GetFrameHeight();
		const ImVec2 size(h - 2.0f, h);
		bool modified = false;

		ui::ScopedID id(label);
		ImGui::PushID(ImGui::GetCurrentTable()->CurrentRow);

		if (AxisButtonX(size, ImGuiButtonFlags_AlignTextBaseLine)) {
			vec.x = ValueType(0);
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::PushID(0);
		if constexpr (isFloat) {
			modified |= ImGui::InputFloat("", &vec.x, step, step_fast, format, flags);
		} else {
			modified |= ImGui::InputInt("", &vec.x, step, step_fast, flags);
		}
		ImGui::PopID();

		if (AxisButtonY(size, ImGuiButtonFlags_AlignTextBaseLine)) {
			vec.y = ValueType(0);
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::PushID(1);
		if constexpr (isFloat) {
			modified |= ImGui::InputFloat("", &vec.y, step, step_fast, format, flags);
		} else {
			modified |= ImGui::InputInt("", &vec.y, step, step_fast, flags);
		}
		ImGui::PopID();

		if (AxisButtonZ(size, ImGuiButtonFlags_AlignTextBaseLine)) {
			vec.z = ValueType(0);
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::PushID(2);
		if constexpr (isFloat) {
			modified |= ImGui::InputFloat("", &vec.z, step, step_fast, format, flags);
		} else {
			ImGui::InputInt("", &vec.z, step, step_fast, flags);
			modified |= ImGui::IsItemDeactivatedAfterEdit();
		}
		ImGui::PopID();

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::TextUnformatted(label);

		ImGui::PopID();

		ImGui::EndGroup();
		return modified;
	}
	if constexpr (isFloat) {
		return InputVec3(label, vec, format, flags);
	} else {
		return InputVec3(label, vec, flags);
	}
}

static core::String varLabel(const core::VarPtr &var) {
	if (var->title().empty()) {
		return var->name();
	}
	return _(var->title().c_str());
}

static void varTooltip(const core::VarPtr &var) {
	if (!var->description().empty()) {
		TooltipTextUnformatted(_(var->description().c_str()));
	}
}

static bool SliderVarInt(const char* label, const core::VarPtr& var, const char* format, ImGuiSliderFlags flags) {
	const int v_min = var->intMinValue();
	const int v_max = var->intMaxValue();
	int val = var->intVal();
	if (SliderInt(label, &val, v_min, v_max, format, flags)) {
		var->setVal(val);
		return true;
	}
	_priv::varTooltip(var);
	return false;
}


} // namespace _priv

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

bool InputVarString(const char *varName, ImGuiInputTextFlags flags) {
	const core::VarPtr &var = core::getVar(varName);
	return InputVarString(var, flags);
}

bool InputVarString(const core::VarPtr &var, ImGuiInputTextFlags flags) {
	const core::String label = _priv::varLabel(var);
	core::String buf = var->strVal();
	flags &= ~ImGuiInputTextFlags_EnterReturnsTrue;
	if (InputText(label.c_str(), &buf, flags)) {
		if (var->setVal(buf)) {
			return true;
		}
	}
	_priv::varTooltip(var);
	return false;
}

void InputFile(const char *label, bool load, core::String *file, const io::FormatDescription *descriptions,
			   ImGuiInputTextFlags flags, const video::FileDialogOptions &options) {
	BeginGroup();
	InputText(label, file, flags);
	SameLine();
	core::String id(ICON_LC_FILE "##");
	id += label;
	if (Button(id.c_str())) {
		if (load) {
			imguiApp()->openDialog([file] (const core::String &filename, const io::FormatDescription *desc) {
				*file = filename;
			}, options, descriptions);
		} else {
			imguiApp()->saveDialog([file] (const core::String &filename, const io::FormatDescription *desc) {
				*file = filename;
			}, options, descriptions, *file);
		}
	}
	EndGroup();
}

void InputFolder(const char *label, core::String *folder, ImGuiInputTextFlags flags) {
	BeginGroup();
	InputText(label, folder, flags);
	SameLine();
	core::String id(ICON_LC_FILE "##");
	id += label;
	if (Button(id.c_str())) {
		imguiApp()->directoryDialog([folder] (const core::String &folderName, const io::FormatDescription *desc) {
			*folder = folderName;
		}, {});
	}
	EndGroup();
}

bool InputVarFloat(const core::VarPtr& var, float step, float step_fast, ImGuiInputTextFlags extra_flags) {
	const core::String label = _priv::varLabel(var);
	float v = var->floatVal();
	if (InputFloat(label.c_str(), &v, step, step_fast, "%.3f", extra_flags)) {
		if (var->setVal(v)) {
			return true;
		}
	}
	_priv::varTooltip(var);
	return false;
}

bool InputVarFloat(const char *varName, float step, float step_fast, ImGuiInputTextFlags extra_flags) {
	core::VarPtr var = core::getVar(varName);
	return InputVarFloat(var, step, step_fast, extra_flags);
}

bool InputVec2(const char *label, glm::ivec2 &vec, ImGuiInputTextFlags flags) {
	return InputInt2(label, glm::value_ptr(vec), flags);
}

bool InputVec2(const char *label, glm::vec2 &vec, const char *format, ImGuiInputTextFlags flags) {
	return InputFloat2(label, glm::value_ptr(vec), format, flags);
}

bool InputVec3(const char *label, const glm::vec3 &vec) {
	return InputVec3(label, const_cast<glm::vec3 &>(vec), "%.3f", ImGuiInputTextFlags_ReadOnly);
}

bool InputVec3(const char *label, glm::vec3 &vec, const char *format, ImGuiInputTextFlags flags) {
	return InputFloat3(label, glm::value_ptr(vec), format, flags);
}

bool InputVec3(const char *label, glm::ivec3 &vec, ImGuiInputTextFlags flags) {
	return InputInt3(label, glm::value_ptr(vec), flags);
}

void AxisStyleText(ui::ScopedStyle &style, math::Axis axis) {
	switch (axis) {
	case math::Axis::X:
		style.setColor(ImGuiCol_Text, style::color(style::ColorAxisX));
		break;
	case math::Axis::Y:
		style.setColor(ImGuiCol_Text, style::color(style::ColorAxisY));
		break;
	case math::Axis::Z:
		style.setColor(ImGuiCol_Text, style::color(style::ColorAxisZ));
		break;
	default:
		break;
	}
}

bool AxisButtonX(const ImVec2& size_arg, ImGuiButtonFlags flags) {
	ui::ScopedStyle style;
	_priv::AxisStyleButton(style, math::Axis::X);
	return ImGui::ButtonEx(_("X"), size_arg, flags);
}

bool AxisButtonY(const ImVec2& size_arg, ImGuiButtonFlags flags) {
	ui::ScopedStyle style;
	_priv::AxisStyleButton(style, math::Axis::Y);
	return ImGui::ButtonEx(_("Y"), size_arg, flags);
}

bool AxisButtonZ(const ImVec2& size_arg, ImGuiButtonFlags flags) {
	ui::ScopedStyle style;
	_priv::AxisStyleButton(style, math::Axis::Z);
	return ImGui::ButtonEx(_("Z"), size_arg, flags);
}

bool AxisCommandButton(math::Axis axis, const char *name, const char *command, const char *icon, const char *tooltip, float width,
					   command::CommandExecutionListener *listener) {
	{
		ui::ScopedStyle style;
		_priv::AxisStyleButton(style, axis);
		char buf[16];
		if (icon != nullptr) {
			core::String::formatBuf(buf, sizeof(buf), "%s %s", icon, name);
		} else {
			core::String::formatBuf(buf, sizeof(buf), "%s", name);
		}
		if (ImGui::Button(buf, ImVec2(width, 0))) {
			if (command::executeCommands(command, listener) > 0) {
				return true;
			}
		}
	}
	if (tooltip != nullptr) {
		ImGui::TooltipTextUnformatted(tooltip);
	} else {
		TooltipCommand(command);
	}
	return false;
}

bool InputAxisInt(math::Axis axis, const char *name, int* value, int step) {
	// TODO: this is changing the + and - buttons and the input field color, not only the description
	// ui::ScopedStyle style;
	// ImGui::AxisStyleText(style, axis);
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
	return ImGui::InputInt(name, value, step);
}

bool CheckboxAxisFlags(math::Axis axis, const char *name, math::Axis* value) {
	ui::ScopedStyle style;
	ImGui::AxisStyleText(style, axis);
	int intvalue = (int)core::enumVal(*value);
	if (ImGui::CheckboxFlags(name, &intvalue, (int)axis)) {
		*value = (math::Axis)intvalue;
		return true;
	}
	return false;
}

bool InputXYZ(const char *label, const glm::vec3 &vec) {
	glm::vec3 copy = vec;
	return InputXYZ(label, copy, "%.3f", ImGuiInputTextFlags_ReadOnly);
}

bool InputXYZ(const char *label, glm::vec3 &vec, const char *format, ImGuiInputTextFlags flags, float step, float step_fast) {
	return _priv::InputXYZImpl(label, vec, format, flags, step, step_fast);
}

bool InputXYZ(const char *label, glm::ivec3 &vec, const char *format, ImGuiInputTextFlags flags, int step, int step_fast) {
	return _priv::InputXYZImpl(label, vec, format, flags, step, step_fast);
}

bool InputFloat(const char *label, float &v, const char *format, ImGuiInputTextFlags flags) {
	if (ImGui::GetCurrentTable()) {
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::PushID(ImGui::GetCurrentTable()->CurrentRow);
		ImGui::InputFloat("##input", &v, 0.0f, 0.0f, format, flags);
		ImGui::PopID();
		bool modified = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(label);
		return modified;
	}
	return ImGui::InputFloat(label, &v, 0.0f, 0.0f, format, flags);
}

bool InputVarInt(const core::VarPtr &var, int step, int step_fast, ImGuiInputTextFlags extra_flags) {
	const core::String label = _priv::varLabel(var);
	int v = var->intVal();
	if (InputInt(label.c_str(), &v, step, step_fast, extra_flags)) {
		if (var->setVal(v)) {
			return true;
		}
	}
	_priv::varTooltip(var);
	return false;
}

bool InputVarInt(const char *varName, int step, int step_fast, ImGuiInputTextFlags extra_flags) {
	core::VarPtr var = core::getVar(varName);
	return InputVarInt(var, step, step_fast, extra_flags);
}

bool IconCheckboxVar(const char *icon, const core::VarPtr &var) {
	const core::String label = _priv::varLabel(var);
	const core::String labelWithIcon(_priv::getId(icon, label.c_str()));
	bool val = var->boolVal();
	if (Checkbox(labelWithIcon.c_str(), &val)) {
		if (var->setVal(val)) {
			return true;
		}
	}
	_priv::varTooltip(var);
	return false;
}

bool CheckboxVar(const core::VarPtr &var) {
	const core::String label = _priv::varLabel(var);
	bool val = var->boolVal();
	if (Checkbox(label.c_str(), &val)) {
		if (var->setVal(val)) {
			return true;
		}
	}
	_priv::varTooltip(var);
	return false;
}

bool IconCheckboxVar(const char *icon, const char *varName) {
	return IconCheckboxVar(icon, core::getVar(varName));
}

bool CheckboxVar(const char *varName) {
	return CheckboxVar(core::getVar(varName));
}

bool IconCheckboxFlags(const char *icon, const char *label, int *flags, int flags_value) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return CheckboxFlags(labelWithIcon.c_str(), flags, flags_value);
}

bool IconCollapsingHeader(const char *icon, const char *label, ImGuiTreeNodeFlags flags) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return CollapsingHeader(labelWithIcon.c_str(), flags);
}

bool IconSliderVarInt(const char *icon, const core::VarPtr &var,
					  const char *format, ImGuiSliderFlags flags) {
	const core::String label = _priv::varLabel(var);
	const core::String labelWithIcon(_priv::getId(icon, label.c_str()));
	return _priv::SliderVarInt(labelWithIcon.c_str(), var, format, flags);
}

bool IconSliderVarInt(const char *icon, const char* varName,
					  const char *format, ImGuiSliderFlags flags) {
	core::VarPtr var = core::getVar(varName);
	return IconSliderVarInt(icon, var, format, flags);
}

bool SliderVarInt(const core::VarPtr& var, const char* format, ImGuiSliderFlags flags) {
	const core::String label = _priv::varLabel(var);
	return _priv::SliderVarInt(label.c_str(), var, format, flags);
}

bool SliderVarInt(const char* varName, const char* format, ImGuiSliderFlags flags) {
	core::VarPtr var = core::getVar(varName);
	return SliderVarInt(var, format, flags);
}

bool SliderVarFloat(const core::VarPtr& var, const char* format, ImGuiSliderFlags flags) {
	const core::String label = _priv::varLabel(var);
	const float v_min = var->floatMinValue();
	const float v_max = var->floatMaxValue();
	float val = var->floatVal();
	if (SliderFloat(label.c_str(), &val, v_min, v_max, format, flags)) {
		var->setVal(val);
		return true;
	}
	_priv::varTooltip(var);
	return false;
}

bool SliderVarFloat(const char *varName, const char *format,
					ImGuiSliderFlags flags) {
	core::VarPtr var = core::getVar(varName);
	return SliderVarFloat(var, format, flags);
}

bool InputVec3Var(const char *varName) {
	glm::vec3 vec;
	const core::VarPtr &var = core::getVar(varName);
	const core::String label = _priv::varLabel(var);
	var->vec3Val(&vec[0]);
	if (InputVec3(label.c_str(), vec, "%.3f", 0)) {
		const core::String &v = core::String::format("%f %f %f", vec.x, vec.y, vec.z);
		var->setVal(v);
		return true;
	}
	_priv::varTooltip(var);
	return false;
}

bool ColorEdit3Var(const char *varName) {
	glm::vec3 col;
	const core::VarPtr &var = core::getVar(varName);
	const core::String label = _priv::varLabel(var);
	var->vec3Val(&col[0]);
	if (ImGui::ColorEdit3(label.c_str(), glm::value_ptr(col))) {
		const core::String &c = core::String::format("%f %f %f", col.x, col.y, col.z);
		var->setVal(c);
		return true;
	}
	_priv::varTooltip(var);
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

void TextWrappedUnformatted(const char *text) {
	ImGuiContext &g = *GImGui;
	const bool need_backup =
		(g.CurrentWindow->DC.TextWrapPos < 0.0f); // Keep existing wrap position if one is already set
	if (need_backup)
		PushTextWrapPos(0.0f);
	ImGuiWindow *window = GetCurrentWindow();
	if (!window->SkipItems)
		TextEx(text, nullptr, ImGuiTextFlags_NoWidthForLargeClippedText);
	if (need_backup)
		PopTextWrapPos();
}

bool TooltipTextUnformatted(const char *text) {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ui::ScopedStyle tooltipStyle;
		tooltipStyle.pushFontSize(imguiApp()->fontSize());
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(text);
		ImGui::EndTooltip();
		return true;
	}
	return false;
}

bool TooltipText(const char *msg, ...) {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::BeginTooltip();

		va_list ap;
		va_start(ap, msg);
		ImGui::TextV(msg, ap);
		va_end(ap);
		ImGui::EndTooltip();
		return true;
	}
	return false;
}

void TextCentered(const char *label, bool reset) {
	const ImVec2 restore = ImGui::GetCursorPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize(label);
	ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
	ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
	ImGui::TextUnformatted(label);
	if (reset) {
		ImGui::SetCursorPos(restore);
	}
}

void Headline(const char *label) {
	ui::ScopedStyle style;
	style.pushFontSize(imguiApp()->bigFontSize());
	ImGui::TextUnformatted(label);
}

// use with e.g. ImGui::InvisibleButton to get the item rect mins and maxs
void AddImage(video::Id handle, const glm::vec2 &uv0, const glm::vec2 &uv1) {
	ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)handle, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
}

void Image(video::Id handle, const glm::ivec2 &size, const glm::vec2 &uv0, const glm::vec2 &uv1, const glm::vec4 &tintColor, const glm::vec4 &borderColor) {
	ImGui::ImageWithBg((ImTextureID)(intptr_t)handle, ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y), ImVec4(borderColor), ImVec4(tintColor));
}

void Image(video::Id handle, const ImVec2 &size, const ImVec2 &uv0, const ImVec2 &uv1, const ImVec4 &tintColor, const ImVec4 &borderColor) {
	ImGui::ImageWithBg((ImTextureID)(intptr_t)handle, size, uv0, uv1, borderColor, tintColor);
}

bool ImageButton(const char *str_id, video::Id handle, const ImVec2 &size) {
	ImVec2 uv0(0, 0);
	ImVec2 uv1(1, 1);
	ImVec4 bgColor(0, 0, 0, 0);
	ImVec4 tintColor(1, 1, 1, 1);
	return ImGui::ImageButton(str_id, (ImTextureID)(intptr_t)handle, size, uv0, uv1, bgColor, tintColor);
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
		const ImVec4 &buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		const ImVec4 &buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImGui::PushStyleColor(ImGuiCol_Button, color::brighter(buttonColor));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color::brighter(buttonHoveredColor));
	}
	const bool pressed = ImGui::Button(label);
	if (state) {
		ImGui::PopStyleColor(2);
	}
	return pressed;
}

bool DisabledButton(const char *label, bool disabled, const ImVec2 &size) {
	if (disabled) {
		const ImVec4 &buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		const ImVec4 &buttonHoveredColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		// TODO: STYLE: if the button color is already gray, the effect is not visible
		ImGui::PushStyleColor(ImGuiCol_Button, color::gray(buttonColor));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color::gray(buttonHoveredColor));
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
		core::String buf = command;
		auto n = buf.find_first_of(' ');
		if (n != core::String::npos) {
			buf = buf.substr(0, n);
		}
		const core::String &help = command::help(buf);
		if (!help.empty()) {
			ImGui::TooltipTextUnformatted(help.c_str());
		}
	}
}

bool CommandButton(const char *label, const char *command, const char *tooltip, const ImVec2 &size, command::CommandExecutionListener* listener) {
	if (ImGui::Button(label, size)) {
		if (command::executeCommands(command, listener) > 0) {
			return true;
		}
	}
	if (tooltip != nullptr) {
		ImGui::TooltipTextUnformatted(tooltip);
	} else {
		TooltipCommand(command);
	}
	return false;
}

bool CommandIconButton(const char *icon, const char *label, const char *command, command::CommandExecutionListener &listener) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return CommandButton(labelWithIcon.c_str(), command, listener);
}

bool CommandButton(const char *label, const char *command, command::CommandExecutionListener &listener) {
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

bool CommandIconMenuItem(const char *icon, const char *label, const char *command, bool enabled, command::CommandExecutionListener* listener) {
	const core::String& keybinding = imguiApp()->getKeyBindingsString(command);
	if (ImGui::MenuItemEx(label, icon, keybinding.c_str(), false, enabled)) {
		if (command::executeCommands(command, listener) > 0) {
			return true;
		}
	}
	TooltipCommand(command);
	return false;
}

bool CommandMenuItem(const char *label, const char *command, bool enabled, command::CommandExecutionListener* listener) {
	return CommandIconMenuItem(nullptr, label, command, enabled, listener);
}

bool CancelButton(const ImVec2 &size) {
	return IconButton(ICON_LC_X, _("Cancel"), size);
}

bool OkButton(const ImVec2 &size) {
	return IconButton(ICON_LC_CHECK, _("Ok"), size);
}

bool YesButton(const ImVec2 &size) {
	return IconButton(ICON_LC_CHECK, _("Yes"), size);
}

bool NoButton(const ImVec2 &size) {
	return IconButton(ICON_LC_X, _("No"), size);
}

bool IconSelectable(const char *icon, const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return Selectable(labelWithIcon.c_str(), selected, flags, size);
}

bool URLIconButton(const char *icon, const char *label, const char *url) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return URLButton(labelWithIcon.c_str(), url);
}

bool URLButton(const char *label, const char *url) {
	const core::String &cmd = core::String::format("url \"%s\"", url);
	if (CommandButton(label, cmd.c_str())) {
		imguiApp()->minimize();
		return true;
	}
	return false;
}

void URLIconItem(const char *icon, const char *label, const char *url, float width) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	URLItem(labelWithIcon.c_str(), url, width);
}

void URLItem(const char *label, const char *url, float width) {
	if (ImGui::TextLink(label)) {
		const core::String &cmd = core::String::format("url \"%s\"", url);
		command::executeCommands(cmd);
	}
	if (ImGui::IsItemHovered()) {
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

	char labelWithIcon[256];
	core::String::formatBuf(labelWithIcon, sizeof(labelWithIcon), "%s %s", icon, label);
	ImGuiID id = window->GetID(label);
	return TreeNodeBehavior(id, flags, labelWithIcon, nullptr);
}

bool Fullscreen(const char *label, ImGuiWindowFlags additionalFlags) {
	SetNextWindowSize(imguiApp()->frameBufferDimension());
	SetNextWindowPos(ImVec2(0.0f, 0.0f));
	return Begin(label, nullptr,
				 additionalFlags | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration |
					 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
					 ImGuiWindowFlags_NoDocking);
}

// https://github.com/ocornut/imgui/issues/1901
bool Spinner(const char *label, float radius, int thickness, const ImU32 &color) {
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext &g = *GImGui;
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ItemSize(bb, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	// Render
	window->DrawList->PathClear();

	int num_segments = 30;
	int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

	const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
	const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

	const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

	for (int i = 0; i < num_segments; i++) {
		const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
		window->DrawList->PathLineTo(
			ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius));
	}

	window->DrawList->PathStroke(color, false, thickness);
	return true;
}

void SetItemTooltipUnformatted(const char *text) {
	if (IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		BeginTooltip();
		TextUnformatted(text);
		EndTooltip();
	}
}

// https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
void LoadingIndicatorCircle(const char *label, const float indicator_radius, const ImVec4 &main_color,
							const ImVec4 &backdrop_color, const int circle_count, const float speed) {
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems) {
		return;
	}

	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::SetCursorPosX((windowSize.x - indicator_radius * 2.0f) * 0.5f);
	ImGui::SetCursorPosY((windowSize.y - indicator_radius * 2.0f) * 0.5f);

	ImGuiContext &g = *GImGui;
	const ImGuiID id = window->GetID(label);

	{
		ui::ScopedStyle style;
		style.pushFontSize(imguiApp()->bigFontSize());
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

void IconDialog(const char *icon, const char *text, bool wrap) {
	ImGui::AlignTextToFramePadding();
	ImGui::PushFont(nullptr, imguiApp()->bigFontSize());
	ImGui::TextUnformatted(icon);
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	if (wrap) {
		ImGui::TextWrappedUnformatted(text);
	} else {
		ImGui::TextUnformatted(text);
	}
	ImGui::Spacing();
	ImGui::Separator();
}

bool IconCheckbox(const char *icon, const char *label, bool *v) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return ImGui::Checkbox(labelWithIcon.c_str(), v);
}

bool BeginIconCombo(const char *icon, const char *label, const char *preview_value, ImGuiComboFlags flags) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return ImGui::BeginCombo(labelWithIcon.c_str(), preview_value, flags);
}

bool BeginIconMenu(const char *icon, const char *label, bool enabled) {
	return BeginMenuEx(label, icon, enabled);
}

bool IconMenuItem(const char *icon, const char *label, const char *shortcut, bool selected, bool enabled) {
	return MenuItemEx(label, icon, shortcut, selected, enabled);
}

bool IconButton(const char *icon, const char *label, const ImVec2 &size) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return Button(labelWithIcon.c_str(), size);
}

bool DisabledIconButton(const char *icon, const char *label, bool disabled, const ImVec2 &size) {
	const core::String labelWithIcon(_priv::getId(icon, label));
	return DisabledButton(labelWithIcon.c_str(), disabled, size);
}

} // namespace ImGui

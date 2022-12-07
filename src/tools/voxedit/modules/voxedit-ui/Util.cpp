/**
 * @file
 */

#include "Util.h"
#include "IMGUIEx.h"
#include "core/Color.h"
#include "core/Enum.h"
#include "imgui.h"

namespace voxedit {
namespace veui {

void AxisStyleButton(ui::ScopedStyle &style, math::Axis axis) {
	const float bright = 0.85f;
	switch (axis) {
	case math::Axis::X:
		style.setColor(ImGuiCol_Text, glm::vec4(1.0f, bright, bright, 1.0f));
		style.setColor(ImGuiCol_Button, core::Color::DarkRed);
		style.setColor(ImGuiCol_ButtonHovered, core::Color::DarkRed);
		style.setColor(ImGuiCol_ButtonActive, core::Color::DarkRed);
		break;
	case math::Axis::Y:
		style.setColor(ImGuiCol_Text, glm::vec4(bright, 1.0f, bright, 1.0f));
		style.setColor(ImGuiCol_Button, core::Color::DarkGreen);
		style.setColor(ImGuiCol_ButtonHovered, core::Color::DarkGreen);
		style.setColor(ImGuiCol_ButtonActive, core::Color::DarkGreen);
		break;
	case math::Axis::Z:
		style.setColor(ImGuiCol_Text, glm::vec4(bright, bright, 1.0f, 1.0f));
		style.setColor(ImGuiCol_Button, core::Color::DarkBlue);
		style.setColor(ImGuiCol_ButtonHovered, core::Color::DarkBlue);
		style.setColor(ImGuiCol_ButtonActive, core::Color::DarkBlue);
		break;
	default:
		break;
	}
}

void AxisStyleText(ui::ScopedStyle &style, math::Axis axis, bool dark) {
	switch (axis) {
	case math::Axis::X:
		style.setColor(ImGuiCol_Text, dark ? core::Color::DarkRed : core::Color::Red);
		break;
	case math::Axis::Y:
		style.setColor(ImGuiCol_Text, dark ? core::Color::DarkGreen : core::Color::Green);
		break;
	case math::Axis::Z:
		style.setColor(ImGuiCol_Text, dark ? core::Color::DarkBlue : core::Color::Blue);
		break;
	default:
		break;
	}
}

const char *AxisButton(math::Axis axis, const char *name, const char *command, const char *icon, const char *tooltip, float width,
					   command::CommandExecutionListener *listener) {
	ui::ScopedStyle style;
	AxisStyleButton(style, axis);
	char buf[16];
	if (icon != nullptr) {
		core::string::formatBuf(buf, sizeof(buf), "%s %s", icon, name);
	} else {
		core::string::formatBuf(buf, sizeof(buf), "%s", name);
	}
	return ImGui::CommandButton(buf, command, tooltip, width, listener);
}

bool InputAxisInt(math::Axis axis, const char *name, int* value, int step) {
	ui::ScopedStyle style;
	//AxisStyleText(style, axis);
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
	return ImGui::InputInt(name, value, step);
}

bool CheckboxAxisFlags(math::Axis axis, const char *name, math::Axis* value) {
	ui::ScopedStyle style;
	AxisStyleText(style, axis, false);
	int intvalue = (int)core::enumVal(*value);
	if (ImGui::CheckboxFlags(name, &intvalue, (int)axis)) {
		*value = (math::Axis)intvalue;
		return true;
	}
	return false;
}

} // namespace veui
} // namespace voxedit

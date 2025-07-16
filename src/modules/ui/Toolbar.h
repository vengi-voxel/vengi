/**
 * @file
 */

#pragma once

#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"
#include "command/CommandHandler.h"
#include "core/String.h"
#include "dearimgui/imgui.h"

namespace ui {

class Toolbar {
protected:
	int _nextId;
	ImVec2 _pos;
	const float _startingPosX;
	const ImVec2 _size;
	command::CommandExecutionListener *_listener;
	const core::String _id;
	int _windowWidth;
	void next();
	void newline();
	void applyIconStyle(ui::ScopedStyle &style);
public:
	Toolbar(const core::String &id, const ImVec2 &size, command::CommandExecutionListener *listener = nullptr);
	Toolbar(const core::String &id, command::CommandExecutionListener *listener = nullptr)
		: Toolbar(id, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), listener) {
	}
	~Toolbar();

	bool button(const char *icon, const char *command, bool highlight = false);

	template<class FUNC>
	bool button(const char *icon, const char *tooltip, FUNC func, bool highlight = false) {
		newline();
		ui::ScopedStyle style;
		applyIconStyle(style);
		if (highlight) {
			style.highlight(ImGuiCol_Text);
		}
		ImGui::PushID(_id.c_str());
		char label[64];
		core::String::formatBuf(label, sizeof(label), "%s###button%d", icon, _nextId);
		bool pressed = ImGui::Button(label, _size);
		if (pressed) {
			func();
		}
		ImGui::PopID();
		if (tooltip != nullptr && tooltip[0] != '\0') {
			ui::ScopedStyle tooltipStyle;
			tooltipStyle.pushFontSize(imguiApp()->fontSize());
			ImGui::TooltipTextUnformatted(tooltip);
		}
		next();
		return pressed;
	}

	template<class FUNC>
	void button(FUNC func, bool applyStyle = true) {
		ui::ScopedStyle style;
		if (applyStyle) {
			applyIconStyle(style);
		}
		newline();
		ImGui::PushID(_id.c_str());
		func(_size);
		ImGui::PopID();
		next();
	}

	// Call this at the end of the toolbar to ensure the last button is handled correctly
	// this is also called in the destructor
	void end();
};

} // namespace ui

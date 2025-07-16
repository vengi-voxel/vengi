/**
 * @file
 */

#include "Toolbar.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"

namespace ui {

Toolbar::Toolbar(const core::String &name, const ImVec2 &size, command::CommandExecutionListener *listener)
	: _nextId(0), _pos(ImGui::GetCursorScreenPos()), _startingPosX(_pos.x), _size(size), _listener(listener), _id(name),
	  _windowWidth(ImGui::GetContentRegionAvail().x) {
}

Toolbar::~Toolbar() {
	end();
}

void Toolbar::next() {
	++_nextId;
	ImGui::SameLine();
	_pos = ImGui::GetCursorScreenPos();
}

void Toolbar::newline() {
	const float wmax = ImGui::GetWindowPos().x + _windowWidth;
	if (_pos.x > _startingPosX && _pos.x + _size.x > wmax) {
		ImGui::NewLine();
		_pos = ImGui::GetCursorScreenPos();
	}
}

void Toolbar::end() {
	newline();
	ImGui::Dummy(ImVec2(0, 0));
}

void Toolbar::applyIconStyle(ui::ScopedStyle &style) {
	style.setFramePadding(ImVec2(1.0f, 1.0f));
	style.setButtonTextAlign(ImVec2(0.5f, 0.5f));
	style.setItemSpacing(ImVec2(1.0f, 1.0f));
	ImGui::AlignTextToFramePadding();
}

bool Toolbar::button(const char *icon, const char *command, bool highlight) {
	newline();
	ui::ScopedStyle style;
	applyIconStyle(style);
	if (highlight) {
		style.highlight(ImGuiCol_Text);
	}
	ImGui::PushID(_id.c_str());
	char label[64];
	core::String::formatBuf(label, sizeof(label), "%s###button%d", icon, _nextId);
	bool pressed = ImGui::CommandButton(label, command, nullptr, _size, _listener);
	ImGui::PopID();
	next();

	return pressed;
}

} // namespace ui

/**
 * @file
 */

#include "Toolbar.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"

namespace ui {

Toolbar::Toolbar(const core::String &name, const ImVec2 &size, command::CommandExecutionListener *listener)
	: _nextId(0), _pos(ImGui::GetCursorScreenPos()), _startingPosX(_pos.x), _size(size), _listener(listener), _id(name) {
}

Toolbar::~Toolbar() {
	last();
}

float Toolbar::windowWidth() const {
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionWidth = available.x + ImGui::GetCursorPosX();
	return contentRegionWidth;
}

void Toolbar::setCursor() {
	ImGui::SetCursorScreenPos(_pos);
}

void Toolbar::next() {
	++_nextId;
	_pos.x += _size.x;
	setCursor();
}

void Toolbar::newline() {
	const float w = windowWidth();
	const float wmax = ImGui::GetWindowPos().x + w;
	if (_pos.x > _startingPosX && _pos.x + _size.x > wmax) {
		_pos.x = _startingPosX;
		_pos.y += _size.y;
		setCursor();
	}
}

void Toolbar::last() {
	const float w = windowWidth();
	const float wmax = ImGui::GetWindowPos().x + w;
	if ((_pos.x > _startingPosX && _pos.x <= wmax) || w < _startingPosX + _size.x) {
		_pos.y += _size.y;
		_pos.x = _startingPosX;
		setCursor();
	}
	ImGui::Dummy(ImVec2(0, 0));
}

bool Toolbar::button(const char *icon, const char *command, bool darken) {
	newline();
	ui::ScopedStyle style;
	// style.setFramePadding(ImVec2(0.0f, 0.0f));
	if (darken) {
		style.darker(ImGuiCol_Text);
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

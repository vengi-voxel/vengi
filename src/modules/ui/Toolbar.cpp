/**
 * @file
 */

#include "Toolbar.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"

namespace ui {

Toolbar::Toolbar(const ImVec2 &size, command::CommandExecutionListener *listener)
	: _pos(ImGui::GetCursorScreenPos()), _startingPosX(_pos.x), _size(size), _listener(listener) {
}

Toolbar::~Toolbar() {
	last();
}

float Toolbar::windowWidth() const {
	return ImGui::GetWindowContentRegionMax().x;
}

void Toolbar::setCursor() {
	ImGui::SetCursorScreenPos(_pos);
}

void Toolbar::next() {
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
}

void Toolbar::button(const char *icon, const char *command) {
	newline();
	ui::ScopedStyle style;
	style.setFramePadding(ImVec2(0.0f, 0.0f));
	ImGui::CommandButton(icon, command, nullptr, _size, _listener);
	next();
}

} // namespace ui

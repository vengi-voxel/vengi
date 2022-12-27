/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "dearimgui/imgui.h"

namespace ui {

class Toolbar {
protected:
	ImVec2 _pos;
	const float _startingPosX;
	const ImVec2 _size;
	command::CommandExecutionListener *_listener;
	float windowWidth() const;
	void setCursor();
	void next();
	void newline();
	void last();

public:
	Toolbar(const ImVec2 &size, command::CommandExecutionListener *listener = nullptr);
	~Toolbar();

	void button(const char *icon, const char *command);
};

} // namespace ui

#pragma once

#include <SDL.h>
#include <vector>
#include <string>
#include "core/Var.h"
#include "TurboBadger.h"

namespace ui {

class Console {
private:
	typedef std::vector<std::string> Messages;
	typedef Messages::const_reverse_iterator MessagesIter;
	Messages _messages;
	bool _consoleActive = false;
	SDL_LogOutputFunction _logFunction = nullptr;
	core::VarPtr _autoEnable;
	tb::TBFontFace *_font;
	std::string _commandLine;

	static void logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message);

public:
	Console();
	bool init();
	bool toggle();
	void render(const tb::TBRect &rect);
	bool onTextInput(const std::string& text);
};

}

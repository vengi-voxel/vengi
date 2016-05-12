#pragma once

#include <SDL.h>
#include <vector>
#include <string>
#include "core/Var.h"

namespace ui {

class Console {
private:
	typedef std::vector<std::string> Messages;
	Messages _messages;
	bool _consoleActive = false;
	SDL_LogOutputFunction _logFunction = nullptr;
	core::VarPtr _autoEnable;

	static void logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message);

public:
	bool init();
	bool toggle();
	void render();
};

}

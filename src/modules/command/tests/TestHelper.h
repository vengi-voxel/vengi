/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/String.h"

namespace command {

class ScopedButtonCommand {
private:
	const core::String _cmd;
	const int _key;
	const double _pressTime;

public:
	ScopedButtonCommand(const core::String &cmd, int key, double pressTime = 0.5)
		: _cmd(cmd), _key(key), _pressTime(pressTime) {
		press();
	}

	~ScopedButtonCommand() {
		release();
	}

	void press() {
		command::executeCommands(core::String::format("+%s %i %f", _cmd.c_str(), _key, _pressTime));
	}

	void release() {
		command::executeCommands(core::String::format("-%s %i %f", _cmd.c_str(), _key, _pressTime + 0.5));
	}
};

} // namespace command

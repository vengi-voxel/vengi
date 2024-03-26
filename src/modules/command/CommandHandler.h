/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace command {

struct CommandExecutionListener {
	virtual ~CommandExecutionListener() {
	}

	virtual bool allowed(const core::String &, const core::DynamicArray<core::String> &) {
		return true;
	}

	core::String command;
	virtual void operator()(const core::String &cmd, const core::DynamicArray<core::String> &) {
		command = cmd;
	}
};

/**
 * @return -1 if the commandline contained anything that couldn't get handled, otherwise the amount of handled commands
 */
extern int executeCommands(const core::String &commandline, CommandExecutionListener *listener = nullptr);

} // namespace command

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
	core::String command;
	virtual void operator()(const core::String &cmd, const core::DynamicArray<core::String> &args) {
		command = cmd;
	}
};

/**
 * @return -1 if the commandline contained anything that couldn't get handled, otherwise the amount of handled commands
 */
extern int executeCommands(const core::String &commandline, CommandExecutionListener *listener = nullptr);

} // namespace command

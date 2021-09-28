/**
 * @file
 */

#pragma once

#include "core/StringUtil.h"

namespace command {

struct CommandExecutionListener {
	virtual void operator()(const core::String &cmd, const core::DynamicArray<core::String> &tokens) {}
};

extern bool replacePlaceholders(const core::String& str, char *buf, size_t bufSize);
/**
 * @return -1 if the commandline contained anything that couldn't get handled, otherwise the amount of handled commands
 */
extern int executeCommands(const core::String& commandline, CommandExecutionListener *listener = nullptr);

}

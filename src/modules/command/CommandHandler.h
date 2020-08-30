/**
 * @file
 */

#pragma once

#include "core/StringUtil.h"

namespace core {

extern bool replacePlaceholders(const core::String& str, char *buf, size_t bufSize);
/**
 * @return -1 if the commandline contained anything that couldn't get handled, otherwise the amount of handled commands
 */
extern int executeCommands(const core::String& commandline);

}

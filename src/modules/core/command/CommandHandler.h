#pragma once

#include <string>

namespace core {

/**
 * @return -1 if the commandline contained anything that couldn't get handled, otherwise the amount of handled commands
 */
extern int executeCommands(const std::string& commandline);

}

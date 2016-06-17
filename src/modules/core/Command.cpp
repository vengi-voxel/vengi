/**
 * @file
 */

#include "Command.h"

namespace core {

Command::CommandMap Command::_cmds;
ReadWriteLock Command::_lock("Command");

int Command::complete(const std::string& str, std::vector<std::string>& matches) const {
	if (_completer) {
		return 0;
	}
	return _completer(str, matches);
}

}

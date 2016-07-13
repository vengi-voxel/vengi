/**
 * @file
 */

#include "Command.h"

namespace core {

Command::CommandMap Command::_cmds;
ReadWriteLock Command::_lock("Command");

Command::Command(std::string&& name, FunctionType&& func) :
		_name(std::move(name)), _func(std::move(func)) {
}

int Command::complete(const std::string& str, std::vector<std::string>& matches) const {
	if (_completer) {
		return 0;
	}
	return _completer(str, matches);
}

int Command::execute(const std::string& command) {
	std::vector<std::string> commands;
	core::string::splitString(command, commands, ";");
	int executed = 0;
	for (const std::string& c : commands) {
		std::vector<std::string> args;
		core::string::splitString(c, args);
		if (args.empty()) {
			continue;
		}
		const std::string cmd = core::string::eraseAllSpaces(args[0]);
		args.erase(args.begin());
		if (execute(cmd, args)) {
			++executed;
		}
	}
	return executed;
}

bool Command::execute(const std::string& command, const CmdArgs& args) {
	ScopedReadLock lock(_lock);
	if ((command[0] == '+' || command[0] == '-') && args.empty()) {
		Log::debug("Skip execution of %s - no arguments provided", command.c_str());
		return false;
	}
	auto i = _cmds.find(command);
	if (i == _cmds.end()) {
		return false;
	}
	const Command& cmd = i->second;
	cmd._func(args);
	return true;
}

Command& Command::registerCommand(std::string&& name, FunctionType&& func) {
	ScopedWriteLock lock(_lock);
	const Command c(std::move(name), std::move(func));
	_cmds.insert(std::make_pair(c.name(), c));
	return _cmds.find(c.name())->second;
}

void Command::unregisterCommand(const std::string& name) {
	ScopedWriteLock lock(_lock);
	_cmds.erase(name);
}

Command& Command::setHelp(const std::string& help) {
	_help = help;
	return *this;
}

Command& Command::setBoolCompleter() {
	return setArgumentCompleter([] (const std::string& str, std::vector<std::string>& matches) -> int {
		if (str[0] == 't') {
			matches.push_back("true");
			return 1;
		}
		if (str[0] == 'f') {
			matches.push_back("false");
			return 1;
		}
		matches.push_back("true");
		matches.push_back("false");
		return 2;
	});
}

}

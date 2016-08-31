/**
 * @file
 */

#include "Command.h"
#include "Tokenizer.h"

namespace core {

Command::CommandMap Command::_cmds;
ReadWriteLock Command::_lock("Command");

Command::Command(const std::string& name, FunctionType&& func) :
		_name(name), _func(std::move(func)) {
}

int Command::complete(const std::string& str, std::vector<std::string>& matches) const {
	if (!_completer) {
		return 0;
	}
	return _completer(str, matches);
}

int Command::execute(const std::string& command) {
	int executed = 0;
	Tokenizer commandLineTokenizer(command, ";");
	while (commandLineTokenizer.hasNext()) {
		const std::string& fullCmd = commandLineTokenizer.next();
		Tokenizer commandTokenizer(fullCmd, " ");
		if (!commandTokenizer.hasNext()) {
			continue;
		}
		const std::string c = commandTokenizer.next();
		std::vector<std::string> args;
		while (commandTokenizer.hasNext()) {
			args.push_back(commandTokenizer.next());
		}
		if (execute(c, args)) {
			++executed;
		}
	}
	return executed;
}

bool Command::execute(const std::string& command, const CmdArgs& args) {
	if ((command[0] == '+' || command[0] == '-') && args.empty()) {
		Log::debug("Skip execution of %s - no arguments provided", command.c_str());
		return false;
	}
	_lock.lockRead();
	auto i = _cmds.find(command);
	if (i == _cmds.end()) {
		_lock.unlockRead();
		return false;
	}
	const Command cmd = i->second;
	_lock.unlockRead();
	cmd._func(args);
	return true;
}

Command& Command::registerCommand(const std::string& name, FunctionType&& func) {
	ScopedWriteLock lock(_lock);
	const Command c(name, std::move(func));
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

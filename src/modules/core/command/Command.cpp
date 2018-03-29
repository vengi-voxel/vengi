/**
 * @file
 */

#include "Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"

namespace core {

Command::CommandMap Command::_cmds;
ReadWriteLock Command::_lock("Command");
std::vector<std::string> Command::_delayedTokens;
int Command::_delayFrames;

int Command::complete(const std::string& str, std::vector<std::string>& matches) const {
	if (!_completer) {
		return 0;
	}
	return _completer(str, matches);
}

int Command::executeDelayed() {
	if (_delayFrames <= 0) {
		_delayFrames = 0;
		return 0;
	}
	if (--_delayFrames > 0) {
		Log::info("Waiting %i frames", _delayFrames);
		return 0;
	}
	// make a copy - it might get modified inside the execute call
	std::vector<std::string> copy = _delayedTokens;
	int executed = 0;
	for (const std::string& fullCmd : copy) {
		Log::info("execute %s", fullCmd.c_str());
		executed += execute(fullCmd);
	}

	return executed;
}

int Command::execute(const std::string& command) {
	int executed = 0;
	Tokenizer commandLineTokenizer(command, ";");
	while (commandLineTokenizer.hasNext()) {
		const std::string& fullCmd = commandLineTokenizer.next();
		if (fullCmd.empty()) {
			continue;
		}
		Log::debug("full command: '%s'", fullCmd.c_str());
		Tokenizer commandTokenizer(fullCmd, " ");
		if (!commandTokenizer.hasNext()) {
			continue;
		}
		const std::string c = commandTokenizer.next();
		Log::debug("command: '%s'", c.c_str());
		std::vector<std::string> args;
		while (commandTokenizer.hasNext()) {
			args.push_back(commandTokenizer.next());
			Log::debug("arg: '%s'", args.back().c_str());
		}
		if (c == "wait") {
			if (args.size() == 1) {
				_delayFrames += std::max(1, core::string::toInt(args[0]));
			} else {
				++_delayFrames;
			}
			++executed;
		}
		if (_delayFrames > 0) {
			while (commandLineTokenizer.hasNext()) {
				const std::string& delayedCommand = commandLineTokenizer.next();
				Log::debug("add command %s to delayed buffer", delayedCommand.c_str());
				_delayedTokens.push_back(delayedCommand);
			}
		} else if (execute(c, args)) {
			++executed;
		}
	}
	return executed;
}

bool Command::execute(const std::string& command, const CmdArgs& args) {
	if (command == "wait") {
		if (args.size() == 1) {
			_delayFrames += std::max(1, core::string::toInt(args[0]));
		} else {
			++_delayFrames;
		}
		return true;
	}
	if ((command[0] == '+' || command[0] == '-') && args.empty()) {
		Log::debug("Skip execution of %s - no arguments provided", command.c_str());
		return false;
	}
	Command cmd;
	{
		ScopedReadLock scoped(_lock);
		auto i = _cmds.find(command);
		if (i == _cmds.end()) {
			Log::debug("could not find command callback for %s", command.c_str());
			return false;
		}
		if (_delayFrames > 0) {
			std::string fullCmd = command;
			for (const std::string& arg : args) {
				fullCmd.append(" ");
				fullCmd.append(arg);
			}
			Log::debug("delay %s", fullCmd.c_str());
			_delayedTokens.push_back(fullCmd);
			return true;
		}
		cmd = i->second;
	}
	Log::debug("execute %s with %i arguments", command.c_str(), (int)args.size());
	cmd._func(args);
	return true;
}

void Command::shutdown() {
	ScopedWriteLock lock(_lock);
	_cmds.clear();
}

bool Command::unregisterCommand(const std::string& name) {
	ScopedWriteLock lock(_lock);
	return _cmds.erase(name) > 0;
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

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
uint64_t Command::_delayMillis = 0;

Command& Command::registerCommand(const char* name, FunctionType&& func) {
	ScopedWriteLock lock(_lock);
	const Command c(name, std::forward<FunctionType>(func));
	_cmds.insert(std::make_pair(name, c));
	return _cmds.find(name)->second;
}

bool Command::unregisterCommand(const char* name) {
	ScopedWriteLock lock(_lock);
	return _cmds.erase(name) > 0;
}

void Command::registerActionButton(const std::string& name, ActionButton& button) {
	ScopedWriteLock lock(_lock);
	const Command cPressed("+" + name, [&] (const core::CmdArgs& args) {
		const int32_t key = core::string::toInt(args[0]);
		const int64_t millis = core::string::toLong(args[1]);
		button.handleDown(key, millis);
	});
	_cmds.insert(std::make_pair(cPressed.name(), cPressed));
	const Command cReleased("-" + name, [&] (const core::CmdArgs& args) {
		const int32_t key = core::string::toInt(args[0]);
		const int64_t millis = core::string::toLong(args[1]);
		button.handleUp(key, millis);
	});
	_cmds.insert(std::make_pair(cReleased.name(), cReleased));
}

bool Command::unregisterActionButton(const std::string& name) {
	ScopedWriteLock lock(_lock);
	int amount = _cmds.erase("-" + name);
	amount += _cmds.erase("+" + name);
	return amount == 2;
}

int Command::complete(const std::string& str, std::vector<std::string>& matches) const {
	if (!_completer) {
		return 0;
	}
	return _completer(str, matches);
}

int Command::update(uint64_t dt) {
	if (_delayMillis == 0) {
		return 0;
	}
	Log::trace("Waiting %i millis", (int)_delayMillis);
	if (dt > _delayMillis) {
		_delayMillis = 0;
	} else {
		_delayMillis -= dt;
	}
	if (_delayMillis > 0) {
		return 0;
	}
	// make a copy - it might get modified inside the execute call
	std::vector<std::string> copy = _delayedTokens;
	_delayedTokens.clear();
	int executed = 0;
	for (const std::string& fullCmd : copy) {
		Log::debug("execute %s", fullCmd.c_str());
		executed += execute(fullCmd);
	}

	return executed;
}

int Command::execute(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[4096];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	buf[sizeof(buf) - 1] = '\0';
	const int cmds = execute(std::string(buf));
	va_end(args);
	return cmds;
}

int Command::execute(const std::string& command) {
	int executed = 0;
	Tokenizer commandLineTokenizer(command, ";\n");
	while (commandLineTokenizer.hasNext()) {
		const std::string& fullCmd = commandLineTokenizer.next();
		if (fullCmd.empty()) {
			continue;
		}
		if (fullCmd[0] == '#') {
			continue;
		}
		if (fullCmd.length() >= 2 && fullCmd[0] == '/' && fullCmd[1] == '/') {
			continue;
		}
		if (_delayMillis > 0) {
			Log::debug("add command %s to delayed buffer", fullCmd.c_str());
			_delayedTokens.push_back(fullCmd);
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
		if (execute(c, args)) {
			++executed;
		}
	}
	return executed;
}

bool Command::execute(const std::string& command, const CmdArgs& args) {
	if (command == "wait") {
		if (args.size() == 1) {
			_delayMillis += core_max(1, core::string::toInt(args[0]));
		} else {
			++_delayMillis;
		}
		return true;
	}
	if ((command[0] == '+' || command[0] == '-') && args.empty()) {
		Log::warn("Skip execution of %s - no arguments provided", command.c_str());
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
		if (_delayMillis > 0) {
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

Command& Command::setHelp(const char* help) {
	if (help == nullptr) {
		_help = "";
	} else {
		_help = help;
	}
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

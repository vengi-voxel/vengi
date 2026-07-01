/**
 * @file
 */

#include "Command.h"
#include "core/ArrayLength.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "core/Trace.h"
#include <SDL3/SDL_stdinc.h>

namespace command {

Command::CommandMap Command::_cmds{MAX_COMMANDS};
core_trace_mutex_static(core::Lock, Command, _lock);
size_t  Command::_sortedCommandListSize = 0u;
Command* Command::_sortedCommandList[MAX_COMMANDS] {};

ActionButtonCommands& ActionButtonCommands::setHelp(const char* help) {
	Command::getCommand(first)->setHelp(help);
	Command::getCommand(second)->setHelp(help);
	return *this;
}

bool Command::isInput() const {
	return _name[0] == COMMAND_PRESSED[0] || _name[0] == COMMAND_RELEASED[0];
}

Command& Command::registerCommand(const core::String &name) {
	const Command c(name);
	core::ScopedLock lock(_lock);
	core_assert(_cmds.size() < MAX_COMMANDS);
	_cmds.put(name, c);
	return (Command&)_cmds.find(name)->value;
}

Command& Command::addArg(const CommandArg &arg) {
	_args.push_back(arg);
	return *this;
}

core::String Command::usage() const {
	core::String msg = "Usage: ";
	msg.append(_name);
	for (const CommandArg &arg : _args) {
		msg.append(" ");
		if (arg.optional) {
			msg.append("[");
		} else {
			msg.append("<");
		}
		msg.append(arg.name);
		if (!arg.defaultVal.empty()) {
			msg.append(":");
			msg.append(arg.defaultVal);
		}
		if (arg.optional) {
			msg.append("]");
		} else {
			msg.append(">");
		}
	}
	return msg;
}

bool Command::parseArgs(const core::DynamicArray<core::String>& rawArgs, CommandArgs& out) const {
	size_t argIdx = 0;
	for (const CommandArg &argDef : _args) {
		if (argIdx < rawArgs.size()) {
			out.set(argDef.name, rawArgs[argIdx]);
		} else if (!argDef.optional) {
			// Required argument missing
			return false;
		} else if (!argDef.defaultVal.empty()) {
			out.set(argDef.name, argDef.defaultVal);
		}
		++argIdx;
	}
	return true;
}

bool Command::unregisterCommand(const core::String &name) {
	core::ScopedLock lock(_lock);
	return _cmds.remove(name);
}

ActionButtonCommands Command::registerActionButton(const core::String& name, ActionButton& button, const core::String &help) {
	core::ScopedLock lock(_lock);
	Command cPressed(COMMAND_PRESSED + name);
	cPressed.addArg({"key", ArgType::Int, true, "0"});
	cPressed.addArg({"seconds", ArgType::Float, true, "0.0"});
	cPressed.setHandler([&] (const CommandArgs& args) {
		const int32_t key = args.intVal("key", 0);
		const double seconds = (double)args.floatVal("seconds", 0.0f);
		button.handleDown(key, seconds);
	});
	cPressed.setHelp(help);
	core_assert(_cmds.size() < MAX_COMMANDS - 1);
	_cmds.put(cPressed.name(), cPressed);
	Command cReleased(COMMAND_RELEASED + name);
	cReleased.addArg({"key", ArgType::Int, true, "0"});
	cReleased.addArg({"seconds", ArgType::Float, true, "0.0"});
	cReleased.setHandler([&] (const CommandArgs& args) {
		const int32_t key = args.intVal("key", 0);
		const double seconds = (double)args.floatVal("seconds", 0.0f);
		button.handleUp(key, seconds);
	});
	cReleased.setHelp(help);
	_cmds.put(cReleased.name(), cReleased);
	return ActionButtonCommands(COMMAND_PRESSED + name, COMMAND_RELEASED + name);
}

bool Command::unregisterActionButton(const core::String& name) {
	core::ScopedLock lock(_lock);
	const core::String downB(COMMAND_PRESSED + name);
	const core::String upB(COMMAND_RELEASED + name);
	int amount = _cmds.remove(downB);
	amount += _cmds.remove(upB);
	return amount == 2;
}

int Command::complete(const core::String& str, core::DynamicArray<core::String>& matches) const {
	if (!_completer) {
		return 0;
	}
	return _completer(str, matches);
}

int Command::completeArg(int argIndex, const core::String& str, const core::Tokens &tokens, core::DynamicArray<core::String>& matches) const {
	if (argIndex < 0 || argIndex >= (int)_args.size()) {
		return 0;
	}
	const CommandArg &arg = _args[argIndex];
	if (arg.completer) {
		return arg.completer(str, tokens, matches);
	}
	// Fall back to the command-level completer for the first argument
	if (argIndex == 0 && _completer) {
		return _completer(str, matches);
	}
	return 0;
}

int Command::update(double deltaFrameSeconds) {
	return 0;
}

void Command::updateSortedList() {
	core::ScopedLock lock(_lock);
	core_assert((int)_cmds.size() < lengthof(_sortedCommandList));
	_sortedCommandListSize = 0u;
	for (auto i = _cmds.begin(); i != _cmds.end(); ++i) {
		_sortedCommandList[_sortedCommandListSize++] = &i->value;
	}
	SDL_qsort(_sortedCommandList, _sortedCommandListSize, sizeof(Command*), [] (const void *v1, const void *v2) {
		return SDL_strcmp((*(const Command*const *)v1)->name().c_str(), (*(const Command*const *)v2)->name().c_str());
	});
}

int Command::execute(const core::String& command) {
	int executed = 0;
	core::TokenizerConfig cfg;
	cfg.skipComments = false;
	core::Tokenizer commandLineTokenizer(cfg, command, ";\n");
	while (commandLineTokenizer.hasNext()) {
		const core::String& fullCmd = commandLineTokenizer.next();
		if (fullCmd.empty()) {
			continue;
		}
		if (fullCmd[0] == '#') {
			continue;
		}
		if (fullCmd.size() >= 2 && fullCmd[0] == '/' && fullCmd[1] == '/') {
			continue;
		}
		Log::trace("full command: '%s'", fullCmd.c_str());
		core::Tokenizer commandTokenizer(cfg, fullCmd, " ");
		if (!commandTokenizer.hasNext()) {
			continue;
		}
		const core::String& c = commandTokenizer.next();
		Log::trace("command: '%s'", c.c_str());
		core::DynamicArray<core::String> args;
		while (commandTokenizer.hasNext()) {
			args.push_back(commandTokenizer.next());
			Log::trace("arg: '%s'", args.back().c_str());
		}
		if (execute(c, args)) {
			++executed;
		}
	}
	return executed;
}

bool Command::execute(const core::String& command, const core::DynamicArray<core::String>& rawArgs) {
	if ((command[0] == COMMAND_PRESSED[0] || command[0] == COMMAND_RELEASED[0]) && rawArgs.empty()) {
		Log::warn("Skip execution of %s - no arguments provided", command.c_str());
		return false;
	}
	Command cmd;
	{
		core::ScopedLock scoped(_lock);
		auto i = _cmds.find(command);
		if (i == _cmds.end()) {
			Log::debug("could not find command callback for %s", command.c_str());
			return false;
		}
		cmd = i->second;
	}
	Log::trace("execute %s with %i arguments", command.c_str(), (int)rawArgs.size());

	// Parse arguments according to command's argument definitions
	CommandArgs parsedArgs;
	if (!cmd.parseArgs(rawArgs, parsedArgs)) {
		Log::info("%s", cmd.usage().c_str());
		return false;
	}

	if (cmd._func) {
		cmd._func(parsedArgs);
	}
	return true;
}

void Command::shutdown() {
	core::ScopedLock lock(_lock);
	_cmds.clear();
}

Command& Command::setHelp(const core::String &help) {
	_help = help;
	return *this;
}

Command& Command::setBoolCompleter() {
	return setArgumentCompleter([] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		if (str[0] == 't') {
			matches.emplace_back("true");
			return 1;
		}
		if (str[0] == 'f') {
			matches.emplace_back("false");
			return 1;
		}
		matches.emplace_back("true");
		matches.emplace_back("false");
		return 2;
	});
}

}

/**
 * @file
 */

#include "Alias.h"
#include "Command.h"
#include "CommandHandler.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "core/Trace.h"
#include "core/Var.h"

namespace command {

Alias::AliasMap Alias::_aliases;
core_trace_mutex_static(core::Lock, Alias, _lock);

namespace {
thread_local int g_aliasExecutionDepth = 0;
const int MaxAliasExecutionDepth = 8;
} // namespace

bool Alias::isValidName(const core::String &name) {
	if (name.empty()) {
		return false;
	}
	if (name == "alias" || name == "unalias") {
		return false;
	}
	if (name[0] == COMMAND_PRESSED[0] || name[0] == COMMAND_RELEASED[0]) {
		return false;
	}
	for (size_t i = 0; i < name.size(); ++i) {
		const char c = name[i];
		if (!core::string::isAlphaNum((int)(unsigned char)c) && c != '_') {
			return false;
		}
	}
	return true;
}

void Alias::construct() {
	command::CommandArg nameArg("name", command::ArgType::String, true, "", "Alias name");
	nameArg.completer = [](const core::String &str, const core::Tokens &, core::DynamicArray<core::String> &matches) -> int {
		int n = 0;
		const core::String pattern = str + "*";
		Alias::visit([&](const core::String &name, const core::String &) {
			if (str.empty() || core::string::matches(name, pattern)) {
				matches.push_back(name);
				++n;
			}
		});
		return n;
	};

	Command::registerCommand("alias")
		.addArg(nameArg)
		.addArg({"commands", ArgType::Rest, true, "", "Commands to execute"})
		.setHandler([](const CommandArgs &args) {
			const core::String &name = args.str("name");
			const core::String &commands = args.str("commands");
			if (name.empty()) {
				if (Alias::size() == 0) {
					Log::info("No aliases defined");
					return;
				}
				Alias::visitSorted([](const core::String &aliasName, const core::String &aliasCommands) {
					Log::info("* %s = \"%s\"", aliasName.c_str(), aliasCommands.c_str());
				});
				return;
			}
			if (commands.empty()) {
				const core::String existing = Alias::get(name);
				if (existing.empty()) {
					Log::info("Unknown alias: %s", name.c_str());
				} else {
					Log::info("* %s = \"%s\"", name.c_str(), existing.c_str());
				}
				return;
			}
			if (!Alias::set(name, commands)) {
				return;
			}
			Log::info("Alias %s = \"%s\"", name.c_str(), commands.c_str());
		})
		.setHelp("Create, update or list command aliases");

	command::CommandArg requiredNameArg("name", command::ArgType::String, false, "", "Alias name");
	requiredNameArg.completer = nameArg.completer;

	Command::registerCommand("unalias")
		.addArg(requiredNameArg)
		.setHandler([](const CommandArgs &args) {
			const core::String &name = args.str("name");
			if (!Alias::remove(name)) {
				Log::info("Unknown alias: %s", name.c_str());
				return;
			}
			Log::info("Removed alias %s", name.c_str());
		})
		.setHelp("Remove a command alias");
}

void Alias::shutdown() {
	core::ScopedLock lock(_lock);
	_aliases.clear();
	g_aliasExecutionDepth = 0;
}

bool Alias::set(const core::String &name, const core::String &commands) {
	if (!isValidName(name)) {
		Log::error("Invalid alias name: %s", name.c_str());
		return false;
	}
	if (commands.empty()) {
		Log::error("Alias '%s' has no commands", name.c_str());
		return false;
	}
	if (Command::getCommand(name) != nullptr && !has(name)) {
		Log::error("Cannot create alias '%s': a command with that name exists", name.c_str());
		return false;
	}
	if (core::findVar(name)) {
		Log::error("Cannot create alias '%s': a variable with that name exists", name.c_str());
		return false;
	}

	{
		core::ScopedLock lock(_lock);
		_aliases.put(name, commands);
	}

	Command::registerCommand(name)
		.setHandler([name](const CommandArgs &) {
			execute(name);
		})
		.setHelp(core::String::format("Alias: %s", commands.c_str()));
	return true;
}

bool Alias::remove(const core::String &name) {
	{
		core::ScopedLock lock(_lock);
		if (!_aliases.remove(name)) {
			return false;
		}
	}
	Command::unregisterCommand(name);
	return true;
}

bool Alias::has(const core::String &name) {
	core::ScopedLock lock(_lock);
	return _aliases.hasKey(name);
}

core::String Alias::get(const core::String &name) {
	core::ScopedLock lock(_lock);
	core::String commands;
	if (!_aliases.get(name, commands)) {
		return core::String::Empty;
	}
	return commands;
}

void Alias::execute(const core::String &name) {
	core::String commands;
	{
		core::ScopedLock lock(_lock);
		if (!_aliases.get(name, commands)) {
			Log::debug("could not find alias %s", name.c_str());
			return;
		}
	}
	if (g_aliasExecutionDepth >= MaxAliasExecutionDepth) {
		Log::error("Alias recursion limit reached while executing '%s'", name.c_str());
		return;
	}
	++g_aliasExecutionDepth;
	executeCommands(commands);
	--g_aliasExecutionDepth;
}

size_t Alias::size() {
	core::ScopedLock lock(_lock);
	return _aliases.size();
}

core::String Alias::serialize() {
	core::String ss;
	ss += "# name \"commands\"\n";
	ss += "# Create aliases in the console with: alias <name> \"command1 ; command2\"\n";
	visitSorted([&](const core::String &name, const core::String &commands) {
		ss += "\"";
		ss += name;
		ss += "\" \"";
		ss += core::string::escape(commands);
		ss += "\"\n";
	});
	return ss;
}

bool Alias::parse(const core::String &content) {
	if (content.empty()) {
		return true;
	}
	core::Tokenizer t(content);
	while (t.hasNext()) {
		const core::String &name = t.next();
		if (name.empty()) {
			Log::warn("Invalid alias name in configuration");
			break;
		}
		if (!t.hasNext()) {
			Log::warn("Missing commands for alias %s", name.c_str());
			break;
		}
		const core::String &commands = t.next();
		if (!set(name, commands)) {
			Log::warn("Failed to restore alias %s", name.c_str());
		}
	}
	return true;
}

} // namespace command

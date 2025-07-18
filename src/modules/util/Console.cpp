/**
 * @file
 */

#include "Console.h"
#include "app/App.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringSet.h"
#include "app/I18N.h"
#include "core/concurrent/Thread.h"
#include "io/Filesystem.h"
#include "command/Command.h"
#include "core/Common.h"
#include "core/Tokenizer.h"
#include "command/CommandHandler.h"
#include "VarUtil.h"
#include <SDL_log.h>
#include <SDL_thread.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_LogSetPriority SDL_SetLogPriority
#define SDL_LogGetPriority SDL_GetLogPriority
#define SDL_LogGetOutputFunction SDL_GetLogOutputFunction
#define SDL_LogSetOutputFunction SDL_SetLogOutputFunction
#endif

namespace util {

static Log::Level toLevel(int priority) {
	Log::Level level = Log::Level::None;
	if (priority == SDL_LOG_PRIORITY_CRITICAL || priority == SDL_LOG_PRIORITY_ERROR) {
		level = Log::Level::Error;
	} else if (priority == SDL_LOG_PRIORITY_WARN) {
		level = Log::Level::Warn;
	} else if (priority == SDL_LOG_PRIORITY_INFO) {
		level = Log::Level::Info;
	} else if (priority == SDL_LOG_PRIORITY_DEBUG) {
		level = Log::Level::Debug;
	} else if (priority == SDL_LOG_PRIORITY_VERBOSE) {
		level = Log::Level::Trace;
	}
	return level;
}

static SDL_LogPriority toPriority(Log::Level level) {
	if (level == Log::Level::Error) {
		return SDL_LOG_PRIORITY_ERROR;
	} else if (level == Log::Level::Warn) {
		return SDL_LOG_PRIORITY_WARN;
	} else if (level == Log::Level::Info) {
		return SDL_LOG_PRIORITY_INFO;
	} else if (level == Log::Level::Debug) {
		return SDL_LOG_PRIORITY_DEBUG;
	}
	// Log::Level::Trace
	return SDL_LOG_PRIORITY_VERBOSE;
}

Console::Console() :
		_mainThread(core::getCurrentThreadId()) {
}

Console::~Console() {
}

void Console::registerOutputCallbacks() {
	SDL_LogGetOutputFunction((SDL_LogOutputFunction*)&_logFunction, &_logUserData);
	SDL_LogSetOutputFunction(logOutputFunction, this);
}

void Console::construct() {
	command::Command::registerCommand("con_clear", [&] (const command::CmdArgs& args) { clear(); }).setHelp(_("Clear the text from the built-in console"));
	command::Command::registerCommand("con_history", [&] (const command::CmdArgs& args) { printHistory(); }).setHelp(_("Print the command history"));
}

bool Console::init() {
	const io::FilesystemPtr& fs = io::filesystem();
	const core::String& content = fs->load("%s", _historyFilename);
	core::string::splitString(content, _history, "\n");
	_historyPos = _history.size();
	Log::debug("Loaded %i history entries", _historyPos);

	return true;
}

void Console::shutdown() {
	core::String content;
	for (const core::String& s : _history) {
		content += s;
		content += '\n';
	}

	const io::FilesystemPtr& fs = io::filesystem();
	if (!fs->homeWrite(_historyFilename, content)) {
		Log::warn("Failed to write the history");
	} else {
		Log::debug("Wrote the history");
	}
	clear();

	command::Command::unregisterCommand("con_clear");
	command::Command::unregisterCommand("con_history");
	SDL_LogSetOutputFunction((SDL_LogOutputFunction)_logFunction, _logUserData);
}

void Console::printHistory() {
	for (const core::String& s : _history) {
		Log::info("%s", s.c_str());
	}
}

void Console::executeCommandLine(command::CommandExecutionListener *listener) {
	_messages.emplace_back(Log::Level::Info, _consolePrompt + _commandLine);
	if (_commandLine.empty()) {
		return;
	}
	_history.push_back(_commandLine);
	_historyPos = _history.size();

	command::executeCommands(_commandLine, listener);
	clearCommandLine();
}

void Console::cursorUp() {
	if (_historyPos <= 0) {
		return;
	}

	--_historyPos;
	_commandLine = _history[_historyPos];
}

void Console::cursorDown() {
	++_historyPos;

	const size_t entries = _history.size();
	if (_historyPos >= entries) {
		_historyPos = entries;
		clearCommandLine();
		return;
	}
	_commandLine = _history[_historyPos];
}

void Console::autoComplete() {
	core::DynamicArray<core::String> matches;
	const core::Tokens allCommands = core::Tokenizer(_commandLine, ";").tokens();
	const core::String& lastCmd = allCommands.empty() ? "" : allCommands.back();
	const core::Tokens strings = core::Tokenizer(lastCmd, " ").tokens();
	core::String baseSearchString = "";
	bool parameter = _commandLine.last() == ' ' || strings.size() > 1;
	if (parameter && !strings.empty()) {
		const command::Command* cmd = command::Command::getCommand(strings.front());
		if (cmd != nullptr) {
			if (strings.back() == strings.front()) {
				cmd->complete("", matches);
			} else {
				cmd->complete(strings.back(), matches);
			}
		}
	} else {
		// try to complete the already existing command
		baseSearchString = strings.empty() ? "" : strings.back();
		command::Command::visitSorted([&] (const command::Command& cmd) {
			if (strings.empty() || strings.size() == 1) {
				// match the command name itself
				if (core::string::matches(cmd.name(), baseSearchString + "*")) {
					matches.push_back(cmd.name());
				}
			} else {
				// match parameters for the command
				cmd.complete(strings.back(), matches);
			}
		});
		if (!strings.empty()) {
			// try to complete the last string as it can be used as a parameter to the command
			baseSearchString = strings.back();
		}
		baseSearchString += '*';
		util::visitVarSorted([&] (const core::VarPtr& var) {
			if (core::string::matches(var->name(), baseSearchString)) {
				matches.push_back(var->name());
			}
		}, 0u);
	}

	if (matches.empty()) {
		return;
	}

	core::StringSet uniqueMatches;
	uniqueMatches.insert(matches.begin(), matches.end());
	matches.clear();
	for (auto i = uniqueMatches.begin(); i != uniqueMatches.end(); ++i) {
		matches.push_back(i->key);
	}
	matches.sort(core::Greater<core::String>());

	if (matches.size() == 1) {
		if (strings.size() <= 1) {
			_commandLine = matches.front() + " ";
		} else {
			const size_t cmdLineSize = _commandLine.size();
			const size_t cmdEraseIndex = cmdLineSize - strings.back().size();
			_commandLine.erase(cmdEraseIndex, strings.back().size());
			_commandLine.insert(cmdEraseIndex, matches.front().c_str());
		}
	} else {
		_messages.emplace_back(Log::Level::Info, _consolePrompt + _commandLine);
		int pos = 0;
		const core::String first = matches.front();
		for (char c : first) {
			bool allMatch = true;
			for (const core::String& match : matches) {
				if (match[pos] != c) {
					allMatch = false;
					break;
				}
			}
			if (!allMatch) {
				break;
			}
			++pos;
		}
		if (pos > 0) {
			replaceLastParameter(matches.front().substr(0, pos));
		}
		for (const core::String& match : matches) {
			Log::info("%s", match.c_str());
		}
	}
}

void Console::replaceLastParameter(const core::String& param) {
	auto iter = _commandLine.rfind(' ');
	if (iter == core::String::npos) {
		_commandLine = param;
		return;
	}

	_commandLine.erase(iter + 1);
	_commandLine.append(param.c_str());
}

core::String Console::removeAnsiColors(const char* message) {
	return core::string::removeAnsiColors(message);
}

void Console::logOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	Console* console = (Console*)userdata;

	Log::Level prio = toLevel(priority);
	if (core::getCurrentThreadId() != console->_mainThread) {
		core_assert(message);
		console->_messageQueue.emplace(category, prio, message);
		return;
	}

	console->addLogLine(category, prio, message);
}

void Console::addLogLine(int category, Log::Level priority, const char *message) {
	_messages.emplace_back(priority, removeAnsiColors(message));
	if (_useOriginalLogFunction) {
		SDL_LogPriority prio = toPriority(priority);
		((SDL_LogOutputFunction)_logFunction)(_logUserData, category, prio, message);
	}
}

void Console::update(double /*deltaFrameSeconds*/) {
	core_assert(_mainThread == core::getCurrentThreadId());
	LogLine msg;
	while (_messageQueue.pop(msg)) {
		core_assert(msg.message);
		addLogLine(msg.category, msg.priority, msg.message);
	}
}

void Console::clear() {
	clearCommandLine();
	_messages.clear();
}

inline void Console::clearCommandLine() {
	_commandLine.clear();
}

}

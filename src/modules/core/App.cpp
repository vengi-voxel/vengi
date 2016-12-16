/**
 * @file
 */

#include "App.h"
#include "Var.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "Common.h"
#include "Log.h"
#include "Tokenizer.h"
#include "Concurrency.h"
#include <thread>
#include <SDL.h>

namespace core {

App* App::_staticInstance;

App::App(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport, size_t threadPoolSize) :
		_trace(traceport), _argc(0), _argv(nullptr), _curState(AppState::Construct), _nextState(AppState::InvalidAppState),
		_suspendRequested(false), _deltaFrame(0L), _initTime(0L), _filesystem(filesystem), _eventBus(eventBus), _threadPool(threadPoolSize, "Core"),
		_timeProvider(timeProvider) {
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
	_now = currentMillis();
	_staticInstance = this;
}

App::~App() {
}

void App::init(const std::string& organisation, const std::string& appname) {
	_organisation = organisation;
	_appname = appname;
}

int App::startMainLoop(int argc, char *argv[]) {
	_argc = argc;
	_argv = argv;

	for (int i = 0; i < _argc; ++i) {
		Log::debug("argv[%i] = %s", i, _argv[i]);
	}

	while (AppState::InvalidAppState != _curState) {
		core_trace_scoped(AppMainLoop);
		onFrame();
	}
	return _exitCode;
}

void App::addBlocker(AppState blockedState) {
	_blockers.insert(blockedState);
}

void App::remBlocker(AppState blockedState) {
	_blockers.erase(blockedState);
}

void App::onFrame() {
	core_trace_begin_frame();
	if (_nextState != AppState::InvalidAppState && _nextState != _curState) {
		if (_blockers.find(_nextState) != _blockers.end()) {
			if (AppState::Blocked != _curState) {
				_curState = AppState::Blocked;
			}
		} else {
			_curState = _nextState;
			_nextState = AppState::InvalidAppState;
		}
	}

	if (AppState::Blocked == _curState) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	} else {
		const long now = currentMillis();
		_deltaFrame = std::max(1l, now - _now);
		_timeProvider->update(now);
		_now = now;

		switch (_curState) {
		case AppState::Construct: {
			core_trace_scoped(AppOnConstruct);
			_nextState = onConstruct();
			break;
		}
		case AppState::Init: {
			core_trace_scoped(AppOnInit);
			_nextState = onInit();
			_nextFrame = currentMillis();
			break;
		}
		case AppState::Running: {
			{
				core_trace_scoped(AppOnRunning);
				if (_framesPerSecondsCap < 1.0 || _nextFrame > now) {
					{
						core_trace_scoped(AppOnBeforeRunning);
						onBeforeRunning();
					}
					const AppState state = onRunning();
					if (_nextState != AppState::Cleanup && _nextState != AppState::Destroy) {
						_nextState = state;
					}
					if (AppState::Running == _nextState) {
						core_trace_scoped(AppOnAfterRunning);
						onAfterRunning();
					}
				}
				if (_framesPerSecondsCap > 1.0) {
					const long delay = _nextFrame - now;
					if (delay > 0) {
						std::this_thread::sleep_for(std::chrono::milliseconds(delay));
					}
					_nextFrame += 1000.0 / _framesPerSecondsCap;
				}
			}
			break;
		}
		case AppState::Cleanup: {
			core_trace_scoped(AppOnCleanup);
			_nextState = onCleanup();
			break;
		}
		case AppState::Destroy: {
			core_trace_scoped(AppOnDestroy);
			_nextState = onDestroy();
			_curState = AppState::InvalidAppState;
			break;
		}
		default:
			break;
		}
	}
	core_trace_end_frame();
}

AppState App::onConstruct() {
	_filesystem->init(_organisation, _appname);

	core::Command::registerCommand("set", [] (const core::CmdArgs& args) {
		if (args.size() != 2) {
			return;
		}
		core::Var::get(args[0], "")->setVal(args[1]);
	}).setHelp("Set a variable name");
	core::Command::registerCommand("varclearhistory", [] (const core::CmdArgs& args) {
		if (args.size() != 1) {
			Log::error("not enough arguments given. Expecting a variable name");
			return;
		}
		const VarPtr& st = core::Var::get(args[0]);
		if (st) {
			st->clearHistory();
		}
	}).setHelp("Clear the value history of a variable");
	core::Command::registerCommand("toggle", [] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::error("not enough arguments given. Expecting a variable name at least");
			return;
		}
		const core::VarPtr& var = core::Var::get(args[0]);
		if (!var) {
			Log::error("given var doesn't exist: %s", args[0].c_str());
			return;
		}
		const uint32_t index = var->getHistoryIndex();
		const uint32_t size = var->getHistorySize();
		if (size <= 1) {
			if (var->typeIsBool()) {
				var->setVal(var->boolVal() ? "false" : "true");
			} else {
				Log::error("Could not toggle %s", args[0].c_str());
			}
			return;
		}
		bool changed;
		if (index == size - 1) {
			changed = var->useHistory(size - 2);
		} else {
			changed = var->useHistory(size - 1);
		}
		if (!changed) {
			Log::error("Could not toggle %s", args[0].c_str());
		}
	}).setHelp("Toggle between true/false for a variable");
	// show a specific variable
	core::Command::registerCommand("show", [] (const core::CmdArgs& args) {
		if (args.size() != 1) {
			Log::error("not enough arguments given. Expecting a variable name");
			return;
		}
		const VarPtr& st = core::Var::get(args[0]);
		if (st) {
			Log::info(" -> %s ", st->strVal().c_str());
		} else {
			Log::info("not found");
		}
	}).setHelp("Show the value of a variable");
	core::Command::registerCommand("cvarlist", [] (const core::CmdArgs& args) {
		core::Var::visitSorted([&] (const core::VarPtr& var) {
			if (!args.empty() && !core::string::matches(args[0], var->name())) {
				return;
			}
			const uint32_t flags = var->getFlags();
			std::string flagsStr = "    ";
			if ((flags & CV_READONLY) != 0) {
				flagsStr[0]  = 'R';
			}
			if ((flags & CV_NOPERSIST) != 0) {
				flagsStr[1]  = 'N';
			}
			if ((flags & CV_SHADER) != 0) {
				flagsStr[2]  = 'S';
			}
			if (var->isDirty()) {
				flagsStr[3]  = 'D';
			}
			const std::string& name = core::string::format("%-28s", var->name().c_str());
			const std::string& value = core::string::format(R"("%s")", var->strVal().c_str());
			Log::info("* %s %s = %s (%u)", flagsStr.c_str(), name.c_str(), value.c_str(), var->getHistorySize());
		});
	}).setHelp("Show the list of known variables (wildcards supported)");
	core::Command::registerCommand("cmdlist", [] (const core::CmdArgs& args) {
		core::Command::visitSorted([&] (const core::Command& cmd) {
			if (!args.empty() && !core::string::matches(args[0], cmd.name())) {
				return;
			}
			Log::info("* %s - %s", cmd.name().c_str(), cmd.help().c_str());
		});
	}).setHelp("Show the list of known commands (wildcards supported)");

	Log::debug("detected %u cpus", core::cpus());

	return AppState::Init;
}

AppState App::onInit() {
	_initTime = _now;

	const std::string& content = _filesystem->load(_appname + ".vars");
	core::Tokenizer t(content);
	while (t.hasNext()) {
		const std::string& name = t.next();
		if (!t.hasNext()) {
			break;
		}
		const std::string& value = t.next();
		if (!t.hasNext()) {
			break;
		}
		const std::string& flags = t.next();
		unsigned int flagsMask = 0u;
		for (char c : flags) {
			if (c == 'R') {
				flagsMask &= CV_READONLY;
			} else if (c == 'S') {
				flagsMask &= CV_SHADER;
			}
		}
		const VarPtr& v = core::Var::get(name, value.c_str(), flagsMask);
		core_assert(v->getFlags() == flagsMask);
	}

	Log::init();
	Log::trace("handle %i command line arguments", _argc);
	for (int i = 0; i < _argc; ++i) {
		// every command is started with a '-'
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' &&_argv[i][1] == '-')) {
			continue;
		}

		const std::string command = &_argv[i][1];
		std::string args;
		args.reserve(256);
		for (++i; i < _argc;) {
			if (_argv[i][0] == '-') {
				--i;
				break;
			}
			args.append(_argv[i++]);
			args.append(" ");
		}
		Log::trace("Execute %s with %i arguments", command.c_str(), (int)args.size());
		core::executeCommands(command + " " + args);
	}
	core::Var::visit([&] (const core::VarPtr& var) {
		var->markClean();
	});
	// we might have changed the loglevel from the commandline
	Log::init();
	_logLevel = core::Var::getSafe(cfg::CoreLogLevel);

	core_trace_init();

	for (int i = 0; i < _argc; ++i) {
		if (!strcmp(_argv[i], "--help") || !strcmp(_argv[i], "-h")) {
			usage();
			return AppState::Destroy;
		}
	}

	return AppState::Running;
}

void App::usage() {
	Log::info("Usage: %s [--help] [-set configvar value] [-commandname]", _appname.c_str());
	Log::info("");
	Log::info("Config variables:");
	core::Var::visitSorted([] (const core::VarPtr& v) {
		Log::info("   %10s %s", v->name().c_str(), v->strVal().c_str());
	});
	Log::info("");
	Log::info("Commands:");
	core::Command::visitSorted([] (const core::Command& c) {
		Log::info("   %10s %s", c.name().c_str(), c.help().c_str());
	});
}

void App::onAfterRunning() {
}

void App::onBeforeRunning() {
}

AppState App::onRunning() {
	if (_logLevel->isDirty()) {
		Log::init();
		_logLevel->markClean();
	}

	core::Command::executeDelayed();

	return AppState::Cleanup;
}

bool App::hasArg(const std::string& arg) const {
	for (int i = 1; i < _argc; ++i) {
		if (arg == _argv[i]) {
			return true;
		}
	}
	return false;
}

std::string App::getArgVal(const std::string& arg) const {
	for (int i = 1; i < _argc; ++i) {
		if (arg != _argv[i]) {
			continue;
		}
		if (i + 1 < _argc) {
			return _argv[i + 1];
		}
	}
	return "";
}

AppState App::onCleanup() {
	if (_suspendRequested) {
		addBlocker(AppState::Init);
		return AppState::Init;
	}

	if (!_organisation.empty() && !_appname.empty()) {
		Log::debug("save the config variables");
		std::stringstream ss;
		core::Var::visitSorted([&](const core::VarPtr& var) {
			if ((var->getFlags() & core::CV_NOPERSIST) != 0u) {
				return;
			}
			const uint32_t flags = var->getFlags();
			std::string flagsStr;
			if ((flags & CV_READONLY) == CV_READONLY) {
				flagsStr.append("R");
			}
			if ((flags & CV_SHADER) == CV_SHADER) {
				flagsStr.append("S");
			}
			ss << R"(")" << var->name() << R"(" ")" << var->strVal() << R"(" ")" << flagsStr << R"(")" << std::endl;
		});
		const std::string& str = ss.str();
		_filesystem->write(_appname + ".vars", str);
	} else {
		Log::warn("don't save the config variables");
	}

	core::Command::shutdown();
	core::Var::shutdown();

	const SDL_AssertData *item = SDL_GetAssertionReport();
	while (item != nullptr) {
		Log::warn("'%s', %s (%s:%d), triggered %u times, always ignore: %s.\n",
				item->condition, item->function, item->filename, item->linenum,
				item->trigger_count, item->always_ignore != 0 ? "yes" : "no");
		item = item->next;
	}
	SDL_ResetAssertionReport();

	core_trace_shutdown();

	return AppState::Destroy;
}

AppState App::onDestroy() {
	return AppState::InvalidAppState;
}

void App::readyForInit() {
	remBlocker(AppState::Init);
}

void App::requestQuit() {
	if (AppState::Running == _curState) {
		_nextState = AppState::Cleanup;
	} else {
		_nextState = AppState::Destroy;
	}
}

void App::requestSuspend() {
	_nextState = AppState::Cleanup;
	_suspendRequested = true;
}

}

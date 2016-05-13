/**
 * @file
 */

#include "App.h"
#include "Var.h"
#include "Command.h"
#include "Log.h"
#include "Tokenizer.h"
#include "Concurrency.h"
#include <thread>
#include <SDL.h>

namespace core {

App* App::_staticInstance;

App::App(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, uint16_t traceport, size_t threadPoolSize) :
		_trace(traceport), _argc(0), _argv(nullptr), _curState(AppState::Construct), _nextState(AppState::InvalidAppState),
		_suspendRequested(false), _deltaFrame(0L), _initTime(0L), _filesystem(filesystem), _eventBus(eventBus), _threadPool(threadPoolSize, "Core") {
	_now = currentMillis();
	_staticInstance = this;
}

App::~App() {
}

void App::init(const std::string& organisation, const std::string& appname) {
	_organisation = organisation;
	_appname = appname;
	core_trace_init();
}

int App::startMainLoop(int argc, char *argv[]) {
	_argc = argc;
	_argv = argv;

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
	core_trace_scoped(AppOnFrame);
	if ((_nextState != AppState::InvalidAppState) && (_nextState != _curState)) {
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
		_deltaFrame = now - _now;
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
			break;
		}
		case AppState::Running: {
			{
				core_trace_scoped(AppOnBeforeRunning);
				onBeforeRunning();
			}
			{
				core_trace_scoped(AppOnRunning);
				_nextState = onRunning();
			}
			if (AppState::Running == _nextState) {
				core_trace_scoped(AppOnAfterRunning);
				onAfterRunning();
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
}

AppState App::onConstruct() {
	_filesystem->init(_organisation, _appname);

	core::Command::registerCommand("set", [] (const core::CmdArgs& args) {
		if (args.size() != 2)
			return;
		core::Var::get(args[0])->setVal(args[1]);
	});
	// show a specific variable
	core::Command::registerCommand("show", [] (const core::CmdArgs& args) {
		if (args.size() != 1)
			return;
		VarPtr st = core::Var::get(args[0]);
		Log::info(" -> %s ",st->strVal().c_str());
	});
	core::Command::registerCommand("cvarlist", [] (const core::CmdArgs& args) {
		core::Var::visit([] (const core::VarPtr& var) {
			const int flags = var->getFlags();
			std::string flagsStr = "  ";
			if (flags & CV_READONLY) {
				flagsStr[0]  = 'R';
			}
			if (!(flags & CV_NOPERSIST)) {
				flagsStr[1]  = 'S';
			}
			const std::string& name = core::string::format("%-28s", var->name().c_str());
			const std::string& value = core::string::format("\"%s\"", var->strVal().c_str());
			Log::info("* %s %s = %s", flagsStr.c_str(), name.c_str(), value.c_str());
		});
	});
	core::Command::registerCommand("cmdlist", [] (const core::CmdArgs& args) {
		core::Command::visit([] (const core::Command& cmd) {
			Log::info("* %s", cmd.name().c_str());
		});
	});

	Log::info("detected %u cpus", core::cpus());

	return AppState::Init;
}

AppState App::onInit() {
	_initTime = _now;

	const std::string& content = _filesystem->load(_appname + ".vars");
	core::Tokenizer t(content);
	while (t.hasNext()) {
		const std::string& name = t.next();
		if (!t.hasNext())
			break;
		const std::string& value = t.next();
		core::Var::get(name)->setVal(value);
	}

	Log::init();
	Log::trace("handle %i command line arguments", _argc);
	for (int i = 0; i < _argc; ++i) {
		// every command is started with a '-'
		if (_argv[i][0] != '-')
			continue;

		const std::string command = &_argv[i][1];
		CmdArgs args;
		for (++i; i < _argc;) {
			if (_argv[i][0] == '-') {
				--i;
				break;
			}
			args.push_back(_argv[i++]);
		}
		Log::trace("Execute %s with %i arguments", command.c_str(), (int)args.size());
		core::Command::execute(command, args);
	}
	// we might have changed the loglevel from the commandline
	Log::init();
	return AppState::Running;
}

void App::onAfterRunning() {
}

void App::onBeforeRunning() {
}

AppState App::onRunning() {
	return AppState::Cleanup;
}

AppState App::onCleanup() {
	if (_suspendRequested) {
		addBlocker(AppState::Init);
		return AppState::Init;
	}

	if (!_organisation.empty() && !_appname.empty()) {
		Log::debug("save the config variables");
		std::stringstream ss;
		core::Var::visit([&](const core::VarPtr& var) {
			if (var->getFlags() & core::CV_NOPERSIST)
				return;
			ss << var->name() << " \"" << var->strVal() << "\"" << std::endl;
		});
		const std::string& str = ss.str();
		_filesystem->write(_appname + ".vars", str);
	} else {
		Log::warn("don't save the config variables");
	}
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

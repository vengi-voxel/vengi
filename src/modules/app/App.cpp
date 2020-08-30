/**
 * @file
 */

#include "App.h"
#include "app/AppCommand.h"
#include "core/Var.h"
#include "core/concurrent/ThreadPool.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "io/Filesystem.h"
#include "core/Common.h"
#include "core/EventBus.h"
#include "metric/Metric.h"
#include "metric/UDPMetricSender.h"
#include "core/Log.h"
#include "core/Tokenizer.h"
#include "core/concurrent/Concurrency.h"
#include "util/VarUtil.h"
#include <SDL.h>
#include "engine-config.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <signal.h>

namespace app {

static void catch_function(int signo) {
	core_stacktrace();
	abort();
}

App* App::_staticInstance;
thread_local std::stack<App::TraceData> App::_traceData;

App* App::getInstance() {
	core_assert(_staticInstance != nullptr);
	return _staticInstance;
}

App::App(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		_filesystem(filesystem), _eventBus(eventBus), _threadPool(std::make_shared<core::ThreadPool>(threadPoolSize, "Core")),
		_timeProvider(timeProvider), _metric(metric) {
	signal(SIGSEGV, catch_function);
	_initialLogLevel = SDL_LOG_PRIORITY_INFO;
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, (SDL_LogPriority)_initialLogLevel);
	_timeProvider->updateTickTime();
	_staticInstance = this;
}

App::~App() {
	core_trace_set(nullptr);
	_metricSender->shutdown();
	_metric->shutdown();
	Log::shutdown();
	_threadPool = core::ThreadPoolPtr();
}

void App::init(const core::String& organisation, const core::String& appname) {
	_organisation = organisation;
	_appname = appname;
}

int App::startMainLoop(int argc, char *argv[]) {
	_argc = argc;
	_argv = argv;

	while (AppState::InvalidAppState != _curState) {
		onFrame();
	}
	return _exitCode;
}

void App::addBlocker(AppState blockedState) {
	_blockers[(int)blockedState] = true;
}

void App::remBlocker(AppState blockedState) {
	_blockers[(int)blockedState] = false;
}

void App::traceBeginFrame(const char *threadName) {
}

void App::traceBegin(const char *threadName, const char* name) {
	_traceData.emplace(TraceData{threadName, name, core::TimeProvider::highResTime()});
}

void App::traceEnd(const char *threadName) {
	if (_traceBlockUntilNextFrame) {
		return;
	}
	if (_traceData.empty()) {
		return;
	}
	TraceData traceData = _traceData.top();
	_traceData.pop();
	const uint64_t dt = core::TimeProvider::highResTime() - traceData.nanos;
	const uint64_t dtMillis = dt / 1000000lu;
	_metric->gauge(traceData.name, (uint32_t)dtMillis, {{"thread", traceData.threadName}});
}

void App::traceEndFrame(const char *threadName) {
	if (!_traceBlockUntilNextFrame) {
		return;
	}
	while (!_traceData.empty()) {
		_traceData.pop();
	}
	_traceBlockUntilNextFrame = false;
}

void App::onFrame() {
	core_trace_begin_frame("Main");
	if (_nextState != AppState::InvalidAppState && _nextState != _curState) {
		if (_blockers[(int)_nextState]) {
			if (AppState::Blocked != _curState) {
				_curState = AppState::Blocked;
			}
		} else {
			_curState = _nextState;
			_nextState = AppState::InvalidAppState;
		}
	}

	_timeProvider->updateTickTime();
	if (AppState::Blocked == _curState) {
		SDL_Delay(1);
		_deltaFrameSeconds = 0.001;
	} else {
		const double now = _timeProvider->tickSeconds();
		_deltaFrameSeconds = now - _nowSeconds;
		_nowSeconds = now;

		switch (_curState) {
		case AppState::Construct: {
			core_trace_scoped(AppOnConstruct);
			_nextState = onConstruct();
			break;
		}
		case AppState::Init: {
			core_trace_scoped(AppOnInit);
			onBeforeInit();
			_nextState = onInit();
			onAfterInit();
			_nextFrameSeconds = now;
			break;
		}
		case AppState::InitFailure: {
			core_trace_scoped(AppOnCleanup);
			_exitCode = 1;
			_nextState = onCleanup();
			break;
		}
		case AppState::Running: {
			core_trace_scoped(AppOnRunning);
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
			const double framesPerSecondsCap = _framesPerSecondsCap->floatVal();
			if (framesPerSecondsCap >= 1.0 && _nextFrameSeconds > now) {
				const double delay = _nextFrameSeconds - now;
				_nextFrameSeconds = now + 1.0 / framesPerSecondsCap;
				if (delay > 0.0) {
					SDL_Delay(delay);
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
	onAfterFrame();
	core_trace_end_frame("Main");
}

AppState App::onConstruct() {
	core::VarPtr logVar = core::Var::get(cfg::CoreLogLevel, _initialLogLevel);
	// this ensures that we are sleeping 1 millisecond if there is enough room for it
	_framesPerSecondsCap = core::Var::get(cfg::CoreMaxFPS, "1000.0");
	registerArg("--loglevel").setShort("-l").setDescription("Change log level from 1 (trace) to 6 (only critical)");
	const core::String& logLevelVal = getArgVal("--loglevel");
	if (!logLevelVal.empty()) {
		logVar->setVal(logLevelVal);
	}
	core::Var::get(cfg::CoreSysLog, _syslog ? "true" : "false");

	Log::init();

	core::Command::registerCommand("set", [] (const core::CmdArgs& args) {
		if (args.size() < 2) {
			return;
		}
		core::Var::get(args[0], "")->setVal(core::string::join(args.begin() + 1, args.end(), " "));
	}).setHelp("Set a variable name");

	core::Command::registerCommand("quit", [&] (const core::CmdArgs& args) {requestQuit();}).setHelp("Quit the application");

	core::Command::registerCommand("core_trace", [&] (const core::CmdArgs& args) {
		if (toggleTrace()) {
			Log::info("Activated statsd based tracing metrics");
		} else {
			Log::info("Deactivated statsd based tracing metrics");
		}
	}).setHelp("Toggle application tracing via statsd");

	AppCommand::init(_timeProvider);

	for (int i = 0; i < _argc; ++i) {
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' && _argv[i][1] == '-')) {
			continue;
		}

		const core::String command = &_argv[i][1];
		if (command != "set") {
			continue;
		}
		if (i + 2 < _argc) {
			core::String var = _argv[i + 1];
			const char *value = _argv[i + 2];
			i += 2;
			core::Var::get(var, value, core::CV_FROMCOMMANDLINE);
			Log::debug("Set %s to %s", var.c_str(), value);
		}
	}

	core::Var::get(cfg::MetricFlavor, "telegraf");
	const core::String& host = core::Var::get(cfg::MetricHost, "127.0.0.1")->strVal();
	const int port = core::Var::get(cfg::MetricPort, "8125")->intVal();
	_metricSender = std::make_shared<metric::UDPMetricSender>(host, port);
	if (!_metricSender->init()) {
		Log::warn("Failed to init metric sender");
		return AppState::Destroy;
	}
	if (!_metric->init(_appname.c_str(), _metricSender)) {
		Log::warn("Failed to init metrics");
		// no hard error...
	}

	Log::init();

	Log::debug("%s: " PROJECT_VERSION, _appname.c_str());

	for (int i = 0; i < _argc; ++i) {
		Log::debug("argv[%i] = %s", i, _argv[i]);
	}

	if (_coredump) {
#ifdef HAVE_SYS_RESOURCE_H
		struct rlimit core_limits;
		core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &core_limits);
		Log::debug("activate core dumps");
#else
		Log::debug("can't activate core dumps");
#endif
	}

	if (!_filesystem->init(_organisation, _appname)) {
		Log::warn("Failed to initialize the filesystem");
	}

	return AppState::Init;
}

bool App::toggleTrace() {
	_traceBlockUntilNextFrame = true;
	if (core_trace_set(this) == this) {
		core_trace_set(nullptr);
		return false;
	}
	return true;
}

void App::onBeforeInit() {
}

AppState App::onInit() {
	SDL_Init(SDL_INIT_TIMER|SDL_INIT_EVENTS);
	_threadPool->init();

	const core::String& content = _filesystem->load(_appname + ".vars");
	core::Tokenizer t(content);
	while (t.hasNext()) {
		const core::String& name = t.next();
		if (!t.hasNext()) {
			break;
		}
		const core::String& value = t.next();
		if (!t.hasNext()) {
			break;
		}
		const core::String& flags = t.next();
		uint32_t flagsMaskFromFile = core::CV_FROMFILE;
		for (char c : flags) {
			if (c == 'R') {
				flagsMaskFromFile |= core::CV_READONLY;
				Log::debug("read only flag for %s", name.c_str());
			} else if (c == 'S') {
				flagsMaskFromFile |= core::CV_SHADER;
				Log::debug("shader flag for %s", name.c_str());
			} else if (c == 'X') {
				flagsMaskFromFile |= core::CV_SECRET;
				Log::debug("secret flag for %s", name.c_str());
			}
		}
		const core::VarPtr& old = core::Var::get(name);
		int32_t flagsMask;
		if (old) {
			flagsMask = (int32_t)(flagsMaskFromFile | old->getFlags());
		} else {
			flagsMask = (int32_t)(flagsMaskFromFile);
		}

		flagsMask &= ~(core::CV_FROMCOMMANDLINE | core::CV_FROMENV);

		core::Var::get(name, value.c_str(), flagsMask);
	}

	Log::init();
	_logLevelVar = core::Var::getSafe(cfg::CoreLogLevel);
	_syslogVar = core::Var::getSafe(cfg::CoreSysLog);

	core::Var::visit([&] (const core::VarPtr& var) {
		var->markClean();
	});

	for (int i = 0; i < _argc; ++i) {
		if (SDL_strcmp(_argv[i], "--help") == 0 || SDL_strcmp(_argv[i], "-h") == 0) {
			usage();
			return AppState::Destroy;
		}
	}

	core_trace_init();

	return AppState::Running;
}

void App::onAfterInit() {
	Log::debug("handle %i command line arguments", _argc);
	for (int i = 0; i < _argc; ++i) {
		// every command is started with a '-'
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' &&_argv[i][1] == '-')) {
			continue;
		}

		const core::String command = &_argv[i][1];
		if (command == "set") {
			// already handled
			continue;
		}
		if (core::Command::getCommand(command) == nullptr) {
			continue;
		}
		core::String args;
		args.reserve(256);
		for (++i; i < _argc;) {
			if (_argv[i][0] == '-') {
				--i;
				break;
			}
			args.append(_argv[i++]);
			args.append(" ");
		}
		Log::debug("Execute %s with %i arguments", command.c_str(), (int)args.size());
		core::executeCommands(command + " " + args);
	}
	const core::String& autoexecCommands = filesystem()->load("autoexec.cfg");
	if (!autoexecCommands.empty()) {
		Log::debug("execute autoexec.cfg");
		core::Command::execute(autoexecCommands);
	} else {
		Log::debug("skip autoexec.cfg");
	}

	const core::String& autoexecAppCommands = filesystem()->load("%s-autoexec.cfg", _appname.c_str());
	if (!autoexecAppCommands.empty()) {
		Log::debug("execute %s-autoexec.cfg", _appname.c_str());
		core::Command::execute(autoexecAppCommands);
	}

	// we might have changed the loglevel from the commandline
	if (_logLevelVar->isDirty() || _syslogVar->isDirty()) {
		Log::init();
		_logLevelVar->markClean();
		_syslogVar->markClean();
	}
}

void App::usage() const {
	const core::VarPtr& logLevel = core::Var::get(cfg::CoreLogLevel, "");
	logLevel->setVal((int)Log::Level::Info);
	Log::init();
	Log::info("Usage: %s [--help] [-set configvar value] [-commandname]", _appname.c_str());
	Log::info("------------");

	int maxWidthLong = 0;
	int maxWidthShort = 0;
	for (const Argument& a : _arguments) {
		maxWidthLong = core_max(maxWidthLong, (int)a.longArg().size());
		maxWidthShort = core_max(maxWidthShort, (int)a.shortArg().size());
	};
	int maxWidthOnlyLong = maxWidthLong + maxWidthShort + 3;
	for (const Argument& a : _arguments) {
		const core::String defaultVal = a.defaultValue().empty() ? "" : core::string::format(" (default: %s)", a.defaultValue().c_str());
		if (a.shortArg().empty()) {
			Log::info("%-*s - %s %s", maxWidthOnlyLong,
				a.longArg().c_str(), a.description().c_str(), defaultVal.c_str());
		} else {
			Log::info("%-*s | %-*s - %s %s", maxWidthLong,
				a.longArg().c_str(), maxWidthShort, a.shortArg().c_str(), a.description().c_str(), defaultVal.c_str());
		}
	}

	int maxWidth = 0;
	core::Var::visit([&] (const core::VarPtr& v) {
		maxWidth = core_max(maxWidth, (int)v->name().size());
	});
	core::Command::visit([&] (const core::Command& c) {
		maxWidth = core_max(maxWidth, (int)SDL_strlen(c.name()));
	});

	Log::info("------------");
	Log::info("Config variables:");
	util::visitVarSorted([=] (const core::VarPtr& v) {
		const uint32_t flags = v->getFlags();
		core::String flagsStr = "     ";
		const char *value = v->strVal().c_str();
		if ((flags & core::CV_READONLY) != 0) {
			flagsStr[0]  = 'R';
		}
		if ((flags & core::CV_NOPERSIST) != 0) {
			flagsStr[1]  = 'N';
		}
		if ((flags & core::CV_SHADER) != 0) {
			flagsStr[2]  = 'S';
		}
		if ((flags & core::CV_SECRET) != 0) {
			flagsStr[3]  = 'X';
			value = "***secret***";
		}
		if (v->isDirty()) {
			flagsStr[4]  = 'D';
		}
		Log::info("   %-*s %s %s", maxWidth, v->name().c_str(), flagsStr.c_str(), value);
		if (v->help() != nullptr) {
			Log::info("   -- %s", v->help());
		}
	}, 0u);
	Log::info("Flags:");
	Log::info("   %-*s Readonly  can't get modified at runtime - only at startup", maxWidth, "R");
	Log::info("   %-*s Nopersist value won't get persisted in the cfg file", maxWidth, "N");
	Log::info("   %-*s Shader    changing the value would result in a recompilation of the shaders", maxWidth, "S");
	Log::info("   %-*s Dirty     the config variable is dirty, means that the initial value was changed", maxWidth, "D");
	Log::info("   %-*s Secret    the value of the config variable won't be shown in the logs", maxWidth, "X");

	Log::info("------------");
	Log::info("Commands:");
	core::Command::visitSorted([=] (const core::Command& c) {
		Log::info("   %-*s %s", maxWidth, c.name(), c.help());
	});
	Log::info("------------");
	Log::info("Config variables can either be set via autoexec.cfg, %s.vars, environment or commandline parameter.", _appname.c_str());
	Log::info("Environment variables must be exported in upper case.");
	Log::info("Examples:");
	Log::info("export the variable CORE_LOGLEVEL with the value 0 to override previous values.");
	Log::info("%s -set core_loglevel 0.", _appname.c_str());
}

void App::onAfterRunning() {
}

void App::onBeforeRunning() {
}

AppState App::onRunning() {
	if (_logLevelVar->isDirty() || _syslogVar->isDirty()) {
		Log::init();
		_logLevelVar->markClean();
		_syslogVar->markClean();
	}

	core::Command::update(_deltaFrameSeconds);

	const int remaining = _eventBus->update(200);
	if (remaining) {
		Log::debug("Remaining events in queue: %i", remaining);
	}
	_filesystem->update();

	return AppState::Cleanup;
}

bool App::hasArg(const core::String& arg) const {
	for (int i = 1; i < _argc; ++i) {
		if (arg == _argv[i]) {
			return true;
		}
	}
	return false;
}

core::String App::getArgVal(const core::String& arg, const core::String& defaultVal, int* argi) {
	int start = argi == nullptr ? 1 : core_max(1, *argi);
	for (int i = start; i < _argc; ++i) {
		if (arg != _argv[i]) {
			continue;
		}
		if (i + 1 < _argc) {
			if (argi != nullptr) {
				*argi = i + 1;
			}
			return _argv[i + 1];
		}
	}
	if (!defaultVal.empty()) {
		return defaultVal;
	}
	for (const Argument& a : _arguments) {
		if (a.longArg() != arg && a.shortArg() != arg) {
			continue;
		}
		for (int i = start; i < _argc; ++i) {
			if (a.longArg() != _argv[i] && a.shortArg() != _argv[i]) {
				continue;
			}
			if (i + 1 < _argc) {
				if (argi != nullptr) {
					*argi = i + 1;
				}
				return _argv[i + 1];
			}
		}
		if (!a.mandatory()) {
			return a.defaultValue();
		}
		if (a.defaultValue().empty()) {
			usage();
			requestQuit();
		}
		return a.defaultValue();
	}
	return "";
}

App::Argument& App::registerArg(const core::String& arg) {
	const App::Argument argument(arg);
	_arguments.insert(argument);
	return *_arguments.back();
}

AppState App::onCleanup() {
	if (_suspendRequested) {
		addBlocker(AppState::Init);
		return AppState::Init;
	}

	// execute all pending async events.
	_eventBus->update();

	if (!_organisation.empty() && !_appname.empty()) {
		Log::debug("save the config variables");
		core::String ss;
		ss.reserve(1024);
		util::visitVarSorted([&](const core::VarPtr& var) {
			if ((var->getFlags() & core::CV_NOPERSIST) != 0u) {
				return;
			}
			const uint32_t flags = var->getFlags();
			core::String flagsStr;
			const char *value = var->strVal().c_str();
			if ((flags & core::CV_READONLY) == core::CV_READONLY) {
				flagsStr.append("R");
			}
			if ((flags & core::CV_SHADER) == core::CV_SHADER) {
				flagsStr.append("S");
			}
			if ((flags & core::CV_SECRET) == core::CV_SECRET) {
				flagsStr.append("X");
			}
			ss += "\"";
			ss += var->name();
			ss += "\" \"";
			ss += value;
			ss += "\" \"";
			ss += flagsStr;
			ss += "\"\n";
		}, 0u);
		_filesystem->write(_appname + ".vars", ss);
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

	_filesystem->shutdown();
	_threadPool->shutdown();

	core_trace_shutdown();

	if (_metricSender) {
		_metricSender->shutdown();
	}
	if (_metric) {
		_metric->shutdown();
	}

#if defined(HAVE_SYS_RESOURCE_H)
#if defined(HAVE_SYS_TIME_H)
	struct rusage usage;
	if (0 == getrusage(RUSAGE_SELF, &usage)) {
		Log::info("Max resident set size used: %li kb", usage.ru_maxrss);
		Log::info("Number of soft page faults: %li", usage.ru_minflt);
		Log::info("Number of page faults: %li", usage.ru_majflt);
		Log::info("Filesystem inputs: %li", usage.ru_inblock);
		Log::info("Filesystem outputs: %li", usage.ru_oublock);
		Log::info("System cpu time: %li ms", usage.ru_stime.tv_sec * 1000L + usage.ru_stime.tv_usec / 1000L);
		Log::info("User cpu time: %li ms", usage.ru_utime.tv_sec * 1000L + usage.ru_utime.tv_usec / 1000L);
	}
#endif
#endif
	SDL_Quit();

	return AppState::Destroy;
}

AppState App::onDestroy() {
	SDL_Quit();
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

const core::String& App::currentWorkingDir() const {
	return _filesystem->basePath();
}

core::ThreadPool& App::threadPool() {
	return *_threadPool.get();
}

}

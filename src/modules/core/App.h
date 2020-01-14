/**
 * @file
 */

#pragma once

#include "Common.h"
#include "metric/Metric.h"
#include "Trace.h"
#include "EventBus.h"
#include "TimeProvider.h"
#include "ThreadPool.h"
#include "BindingContext.h"
#include <stack>
#include <atomic>

#define ORGANISATION "vengi"

namespace io {
class Filesystem;
typedef std::shared_ptr<Filesystem> FilesystemPtr;
}

namespace core {

class Var;
typedef std::shared_ptr<Var> VarPtr;


enum class AppState : uint8_t {
	Construct,
	Init,
	InitFailure,
	Running,
	Cleanup,
	Destroy,
	Blocked,
	NumAppStates,
	InvalidAppState,
	Max
};

/**
 * @brief The app class controls the main loop of every application.
 */
class App : public core::TraceCallback {
protected:
	core::Trace _trace;
	int _argc = 0;
	char **_argv = nullptr;

	int _initialLogLevel = SDL_LOG_PRIORITY_INFO;

	std::string _organisation;
	std::string _appname;

	BindingContext _bindingContext = BindingContext::All;

	AppState _curState = AppState::Construct;
	AppState _nextState = AppState::InvalidAppState;
	bool _blockers[(int)AppState::Max] { false, false, false, false, false, false, false, false, false };
	bool _suspendRequested = false;

	/**
	 * @brief Should the application log to the syslog daemon
	 */
	bool _syslog = false;
	/**
	 * @brief Should the application generate a core dump on a crash
	 */
	bool _coredump = false;
	/**
	 * @brief A cached value of the @c core::TimeProvider tick milliseconds.
	 */
	uint64_t _now;
	/**
	 * @brief The millisecond delta of the start of the current frame
	 * and the start of the last frame
	 */
	uint64_t _deltaFrameMillis = 0ul;
	float _deltaFrameSeconds = 0.0f;
	/**
	 * @brief The absolute milliseconds when the application was started.
	 * Can be used to calculate the uptime.
	 */
	uint64_t _initMillis = 0ul;
	/**
	 * @brief The absolute milliseconds when the next frame should be run
	 * @note Only handled if the max frames cap is set
	 */
	uint64_t _nextFrameMillis = 0ul;
	/**
	 * @brief There is no fps limit per default, but you set one on a per-app basis
	 * @param[in] framesPerSecondsCap The frames to cap the application loop at
	 */
	core::VarPtr _framesPerSecondsCap;

	/**
	 * @brief If the application failed to init or must be closed due to a failure, you
	 * can set the exit code to expose the reason to the console that called the application.
	 */
	int _exitCode = 0;

	static App* _staticInstance;

	io::FilesystemPtr _filesystem;
	core::EventBusPtr _eventBus;
	core::ThreadPool _threadPool;
	core::TimeProviderPtr _timeProvider;
	core::VarPtr _logLevelVar;
	core::VarPtr _syslogVar;
	metric::IMetricSenderPtr _metricSender;
	metric::MetricPtr _metric;
	// if you modify the tracing during the frame, we throw away the current frame information
	std::atomic_bool _traceBlockUntilNextFrame { false };
	struct TraceData {
		const char *threadName;
		const char *name;
		uint64_t nanos;
	};
	static thread_local std::stack<TraceData> _traceData;

	bool toggleTrace();

	virtual void traceBeginFrame(const char *threadName) override;
	virtual void traceBegin(const char *threadName, const char* name) override;
	virtual void traceEnd(const char *threadName) override;
	virtual void traceEndFrame(const char *threadName) override;

	void usage() const;

public:
	App(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~App();

	void init(const std::string& organisation, const std::string& appname);
	int startMainLoop(int argc, char *argv[]);

	/**
	 * @brief Register your commands and cvars here
	 * @note The registered commands and cvars are automatically printed in --help. If you register them in a later application state,
	 * this is no longer the case.
	 * @return @c AppState::Init as next phase
	 */
	virtual AppState onConstruct();
	virtual void onBeforeInit();
	/**
	 * @brief Evaluates the command line parameters that the application was started with.
	 * @note Make sure your commands are already registered (@see onConstruct())
	 * @return @c AppState::Running if initialization succeeds, @c AppState::InitFailure if it failed.
	 */
	virtual AppState onInit();
	virtual void onAfterInit();
	virtual void onBeforeRunning();
	/**
	 * @brief called every frame after the initialization was done
	 */
	virtual AppState onRunning();
	virtual void onAfterRunning();
	virtual AppState onCleanup();
	virtual AppState onDestroy();

	/**
	 * @brief Don't enter the given @c AppState before the blocker was removed. This can be used to implement
	 * e.g. long initialization phases.
	 * @see @c remBlocker()
	 */
	void addBlocker(AppState blockedState);
	/**
	 * @brief Indicate that the given @c AppState can now be entered.
	 * @see @c addBlocker()
	 */
	void remBlocker(AppState blockedState);

	const std::string& appname() const;

	class Argument {
	private:
		std::string _longArg;
		std::string _shortArg;
		std::string _description;
		std::string _defaultValue;
		bool _mandatory = false;

	public:
		Argument(const std::string& longArg) :
				_longArg(longArg) {
		}

		Argument& setShort(const std::string& shortArg) {
			_shortArg = shortArg;
			return *this;
		}

		Argument& setMandatory() {
			_mandatory = true;
			return *this;
		}

		Argument& setDescription(const std::string& description) {
			_description = description;
			return *this;
		}

		Argument& setDefaultValue(const std::string& defaultValue) {
			_defaultValue = defaultValue;
			return *this;
		}

		inline const std::string& defaultValue() const {
			return _defaultValue;
		}

		inline const std::string& description() const {
			return _description;
		}

		inline const std::string& longArg() const {
			return _longArg;
		}

		inline bool mandatory() const {
			return _mandatory;
		}

		inline const std::string& shortArg() const {
			return _shortArg;
		}
	};

	/**
	 * @note Only valid after
	 */
	bool hasArg(const std::string& arg) const;
	std::string getArgVal(const std::string& arg, const std::string& defaultVal = "", int* argi = nullptr);
	Argument& registerArg(const std::string& arg);

	// handle the app state changes here
	virtual void onFrame();
	virtual void onAfterFrame() {}
	void readyForInit();
	void requestQuit();
	void requestSuspend();

	uint64_t deltaFrame() const;
	uint64_t lifetimeInSeconds() const;
	float lifetimeInSecondsf() const;

	AppState state() const;

	/**
	 * @return the millis since the epoch
	 */
	uint64_t systemMillis() const;

	/**
	 * @brief Access to the FileSystem
	 */
	io::FilesystemPtr filesystem() const;

	core::ThreadPool& threadPool();

	/**
	 * @brief Access to the global TimeProvider
	 */
	core::TimeProviderPtr timeProvider() const;

	metric::MetricPtr metric() const;

	/**
	 * @brief Access to the global EventBus
	 */
	core::EventBusPtr eventBus() const;

	const std::string& currentWorkingDir() const;

	/**
	 * @brief Allows to change the binding context. This can be used to e.g. ignore some commands while hovering
	 * the ui and they should only be active if the scene has the focus.
	 * @return the old context
	 * @sa bindingContext()
	 */
	BindingContext setBindingContext(BindingContext newInputContext);

	/**
	 * @brief Get the current binding context
	 * @sa setBindingContext() for more details.
	 */
	BindingContext bindingContext() const;

	static App* getInstance();

private:
	std::vector<Argument> _arguments;
};

inline uint64_t App::lifetimeInSeconds() const {
	return (_now - _initMillis) / uint64_t(1000);
}

inline float App::lifetimeInSecondsf() const {
	return float(_now - _initMillis) / 1000.0f;
}

inline uint64_t App::deltaFrame() const {
	return _deltaFrameMillis;
}

inline uint64_t App::systemMillis() const {
	return _timeProvider->systemMillis();
}

inline io::FilesystemPtr App::filesystem() const {
	return _filesystem;
}

inline core::TimeProviderPtr App::timeProvider() const {
	return _timeProvider;
}

inline metric::MetricPtr App::metric() const {
	return _metric;
}

inline core::ThreadPool& App::threadPool() {
	return _threadPool;
}

inline core::EventBusPtr App::eventBus() const {
	return _eventBus;
}

inline const std::string& App::appname() const {
	return _appname;
}

inline BindingContext App::bindingContext() const {
	return _bindingContext;
}

inline AppState App::state() const {
	return _curState;
}

}

namespace io {

inline io::FilesystemPtr filesystem() {
	return core::App::getInstance()->filesystem();
}

}

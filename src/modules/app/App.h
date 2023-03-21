/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/SharedPtr.h"
#include "io/Filesystem.h"
#include "core/TimeProvider.h"

#define ORGANISATION "vengi"

/**
 * Foundation classes
 */
namespace core {
class ThreadPool;
typedef core::SharedPtr<ThreadPool> ThreadPoolPtr;

class Var;
typedef core::SharedPtr<Var> VarPtr;

class TimeProvider;
typedef core::SharedPtr<TimeProvider> TimeProviderPtr;
}

namespace app {
/**
 * These are the various app states of the lifecycle
 */
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
 * @brief The app class controls the main loop and life-cycle of every application.
 */
class App : public core::TraceCallback {
protected:
	core::Trace _trace;
	// the amount of command line arguments
	int _argc = 0;
	// the first entry is the application name
	char **_argv = nullptr;

	int _initialLogLevel = 0;

	core::String _organisation;
	core::String _appname;
	core::String _additionalUsage; //!< allow to specify options or filenames for the usage() screen per app */

	AppState _curState = AppState::Construct;
	AppState _nextState = AppState::InvalidAppState;
	bool _blockers[(int)AppState::Max] { false, false, false, false, false, false, false, false, false };
	bool _suspendRequested = false;
	bool _failedToSaveConfiguration = false;

	/**
	 * @brief Should the application log to the syslog daemon
	 */
	bool _syslog = false;
	/**
	 * @brief Should the application generate a core dump on a crash
	 */
	bool _coredump = false;
	/**
	 * @brief The seconds delta of the start of the current frame
	 * and the start of the last frame
	 */
	double _deltaFrameSeconds = 0.0f;
	double _nowSeconds = 0.0f;
	/**
	 * @brief The absolute seconds when the next frame should be run
	 * @note Only handled if the max frames cap is set
	 */
	double _nextFrameSeconds = 0ul;
	/**
	 * @brief There is no fps limit per default, but you set one on a per-app basis
	 * The frames to cap the application loop at
	 */
	core::VarPtr _framesPerSecondsCap;

	/**
	 * @brief If the application failed to init or must be closed due to a failure, you
	 * can set the exit code to expose the reason to the console that called the application.
	 */
	int _exitCode = 0;

	static App* _staticInstance;

	io::FilesystemPtr _filesystem;
	core::ThreadPoolPtr _threadPool;
	core::TimeProviderPtr _timeProvider;
	core::VarPtr _logLevelVar;
	core::VarPtr _syslogVar;

	bool toggleTrace();

	virtual void traceBeginFrame(const char *threadName) override;
	virtual void traceBegin(const char *threadName, const char* name) override;
	virtual void traceEnd(const char *threadName) override;
	virtual void traceEndFrame(const char *threadName) override;

	virtual void usage() const;
	void setArgs(int argc, char *argv[]);

#ifdef __EMSCRIPTEN__
	static void runFrameEmscripten();
#endif

public:
	App(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~App();

	void init(const core::String& organisation, const core::String& appname);
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

	bool saveConfiguration();

	const core::String& appname() const;

	class Argument {
	private:
		core::String _longArg;
		core::String _shortArg;
		core::String _description;
		core::String _defaultValue;
		bool _mandatory = false;

	public:
		Argument(const core::String& longArg) :
				_longArg(longArg) {
		}

		Argument& setShort(const core::String& shortArg) {
			_shortArg = shortArg;
			return *this;
		}

		Argument& setMandatory() {
			_mandatory = true;
			return *this;
		}

		Argument& setDescription(const core::String& description) {
			_description = description;
			return *this;
		}

		Argument& setDefaultValue(const core::String& defaultValue) {
			_defaultValue = defaultValue;
			return *this;
		}

		inline const core::String& defaultValue() const {
			return _defaultValue;
		}

		inline const core::String& description() const {
			return _description;
		}

		inline const core::String& longArg() const {
			return _longArg;
		}

		inline bool mandatory() const {
			return _mandatory;
		}

		inline const core::String& shortArg() const {
			return _shortArg;
		}
	};

	/**
	 * @note Only valid after
	 */
	bool hasArg(const core::String& arg) const;
	core::String getArgVal(const core::String& arg, const core::String& defaultVal = "", int* argi = nullptr);
	Argument& registerArg(const core::String& arg);

	// handle the app state changes here
	virtual void onFrame();
	virtual void onAfterFrame() {}
	/**
	 * If you have e.g. unsaved or any other reason to prevent the shutdown of the application, you can return @c false here
	 */
	virtual bool allowedToQuit();
	void readyForInit();
	void requestQuit();
	void requestSuspend();

	double deltaFrameSeconds() const;
	double nowSeconds() const;

	AppState state() const;
	bool shouldQuit() const;

	/**
	 * @brief Access to the FileSystem
	 */
	io::FilesystemPtr filesystem() const;

	core::ThreadPool& threadPool();

	/**
	 * @brief Access to the global TimeProvider
	 */
	core::TimeProviderPtr timeProvider() const;

	const core::String& currentWorkingDir() const;

	static App* getInstance();

private:
	core::DynamicArray<Argument> _arguments;
};

inline double App::nowSeconds() const {
	return _nowSeconds;
}

inline double App::deltaFrameSeconds() const {
	return _deltaFrameSeconds;
}

inline io::FilesystemPtr App::filesystem() const {
	return _filesystem;
}

inline core::TimeProviderPtr App::timeProvider() const {
	return _timeProvider;
}

inline const core::String& App::appname() const {
	return _appname;
}

inline AppState App::state() const {
	return _curState;
}

inline bool App::shouldQuit() const {
	return _nextState == AppState::Cleanup || _nextState == AppState::Destroy;
}

}

namespace io {

inline io::FilesystemPtr filesystem() {
	return app::App::getInstance()->filesystem();
}

}

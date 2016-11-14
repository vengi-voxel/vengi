/**
 * @file
 */

#pragma once

#include <unordered_set>
#include <chrono>
#include "Common.h"
#include "Var.h"
#include "Trace.h"
#include "EventBus.h"
#include "TimeProvider.h"
#include "core/ThreadPool.h"
#include "io/Filesystem.h"

namespace core {

enum AppState {
	Construct, Init, Running, Cleanup, Destroy, Blocked, NumAppStates, InvalidAppState,
};

/**
 * @brief The app class controls the main loop of every application.
 */
class App {
protected:
	core::Trace _trace;
	int _argc;
	char **_argv;

	std::string _organisation;
	std::string _appname;

	AppState _curState;
	AppState _nextState;
	std::unordered_set<AppState, std::hash<int> > _blockers;
	bool _suspendRequested;
	long _now;
	long _deltaFrame;
	long _initTime;
	double _nextFrame = 0;
	double _framesPerSecondsCap = 0.0;
	int _exitCode = 0;
	io::FilesystemPtr _filesystem;
	core::EventBusPtr _eventBus;
	static App* _staticInstance;
	core::ThreadPool _threadPool;
	core::TimeProviderPtr _timeProvider;
	core::VarPtr _logLevel;

	/**
	 * @brief There is no fps limit per default, but you set one on a per-app basis
	 * @param[in] framesPerSecondsCap The frames to cap the application loop at
	 */
	void setFramesPerSecondsCap(double framesPerSecondsCap) {
		_framesPerSecondsCap = framesPerSecondsCap;
	}

public:
	App(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport, size_t threadPoolSize = 1);
	virtual ~App();

	void init(const std::string& organisation, const std::string& appname);
	int startMainLoop(int argc, char *argv[]);

	// e.g. register your commands here
	virtual AppState onConstruct();
	// evaluates the command line parameters that the application was started with. Make sure your commands are already registered
	virtual AppState onInit();
	virtual void onBeforeRunning();
	// called every frame after the initalization was done
	virtual AppState onRunning();
	virtual void onAfterRunning();
	virtual AppState onCleanup();
	virtual AppState onDestroy();

	void addBlocker(AppState blockedState);
	void remBlocker(AppState blockedState);

	const std::string& appname() const;

	/**
	 * @note Only valid after
	 */
	bool hasArg(const std::string& arg) const;
	std::string getArgVal(const std::string& arg) const;

	// handle the app state changes here
	virtual void onFrame();
	void readyForInit();
	void requestQuit();
	void requestSuspend();

	long deltaFrame() const;

	/**
	 * @return the millis since the epoch
	 */
	long currentMillis() const;

	/**
	 * @brief Access to the FileSystem
	 */
	io::FilesystemPtr filesystem() const;

	core::ThreadPool& threadPool();

	/**
	 * @brief Access to the global TimeProvider
	 */
	core::TimeProviderPtr timeProvider() const;

	/**
	 * @brief Access to the global EventBus
	 */
	core::EventBusPtr eventBus() const;

	std::string currentWorkingDir() const;

	static App* getInstance() {
		core_assert(_staticInstance != nullptr);
		return _staticInstance;
	}
};

inline long App::deltaFrame() const {
	return _deltaFrame;
}

inline long App::currentMillis() const {
	return _timeProvider->currentTime();
}

inline io::FilesystemPtr App::filesystem() const {
	return _filesystem;
}

inline core::TimeProviderPtr App::timeProvider() const {
	return _timeProvider;
}

inline core::ThreadPool& App::threadPool() {
	return _threadPool;
}

inline core::EventBusPtr App::eventBus() const {
	return _eventBus;
}

inline std::string App::currentWorkingDir() const {
	return _filesystem->basePath();
}

inline const std::string& App::appname() const {
	return _appname;
}

}

#pragma once

#include <unordered_set>
#include <chrono>
#include "Trace.h"
#include "EventBus.h"
#include "io/Filesystem.h"

namespace core {

enum AppState {
	Construct, Init, Running, Cleanup, Destroy, Blocked, NumAppStates, InvalidAppState,
};

class App {
private:
	//std::streambuf* _oldOut;
	//std::streambuf* _oldErr;

protected:
	int _argc;
	char **_argv;

	std::string _organisation;
	std::string _appname;

	Trace _trace;
	AppState _curState;
	AppState _nextState;
	std::unordered_set<AppState, std::hash<int> > _blockers;
	bool _suspendRequested;
	long _now;
	long _deltaFrame;
	long _initTime;
	io::FilesystemPtr _filesystem;
	core::EventBusPtr _eventBus;
	static App* _staticInstance;

public:
	App(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, uint16_t traceport);
	virtual ~App();

	void init(const std::string& organisation, const std::string& appname);
	void startMainLoop(int argc, char *argv[]);

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

	// handle the app state changes here
	virtual void onFrame();
	void readyForInit();
	void requestQuit();
	void requestSuspend();

	/**
	 * @return the millis since the epoch
	 */
	long currentMillis() const;

	io::FilesystemPtr filesystem() const;

	core::EventBusPtr eventBus() const;

	static App* getInstance() {
		core_assert(_staticInstance != nullptr);
		return _staticInstance;
	}
};

inline long App::currentMillis() const {
	return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

inline io::FilesystemPtr App::filesystem() const {
	return _filesystem;
}

inline core::EventBusPtr App::eventBus() const {
	return _eventBus;
}

}

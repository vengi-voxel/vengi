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

#define ORGANISATION "engine"

namespace io {
class Filesystem;
typedef std::shared_ptr<Filesystem> FilesystemPtr;
}

namespace core {

enum class AppState : uint8_t {
	Construct,
	Init,
	Running,
	Cleanup,
	Destroy,
	Blocked,
	NumAppStates,
	InvalidAppState,
};

/**
 * @brief The app class controls the main loop of every application.
 */
class App {
protected:
	class ProfilerCPU {
	private:
		double _min = 0.0;
		double _max = 0.0;
		double _avg = 0.0;
		std::string _name;
		std::vector<double> _samples;
		const int16_t _maxSampleCount;
		int16_t _sampleCount = 0;
		double _stamp = 0.0;
	public:
		ProfilerCPU(const std::string& name, uint16_t maxSamples = 1024u);
		const std::vector<double>& samples() const;
		void enter();
		void leave();
		double minimum() const;
		double maximum() const;
		double avg() const;
		const std::string& name() const;
	};

	template<class Profiler>
	struct ScopedProfiler {
		Profiler& _p;
		inline ScopedProfiler(Profiler& p) : _p(p) {
			p.enter();
		}
		inline ~ScopedProfiler() {
			_p.leave();
		}
	};

	core::Trace _trace;
	int _argc;
	char **_argv;

	std::string _organisation;
	std::string _appname;

	AppState _curState;
	AppState _nextState;
	std::unordered_set<AppState> _blockers;
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

	void usage();

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
	float lifetimeInSeconds() const;

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

	const std::string& currentWorkingDir() const;

	static App* getInstance() {
		core_assert(_staticInstance != nullptr);
		return _staticInstance;
	}
};

inline App::ProfilerCPU::ProfilerCPU(const std::string& name, uint16_t maxSamples) :
		_name(name), _maxSampleCount(maxSamples) {
	core_assert(maxSamples > 0);
	_samples.reserve(_maxSampleCount);
}

inline const std::vector<double>& App::ProfilerCPU::samples() const {
	return _samples;
}

inline void App::ProfilerCPU::enter() {
	_stamp = core::TimeProvider::currentNanos();
}

inline void App::ProfilerCPU::leave() {
	const double time = core::TimeProvider::currentNanos() - _stamp;
	_max = std::max(_max, time);
	_min = std::min(_min, time);
	_avg = _avg * 0.5 + time * 0.5;
	_samples[_sampleCount & (_maxSampleCount - 1)] = time;
	++_sampleCount;
}

inline const std::string& App::ProfilerCPU::name() const {
	return _name;
}

inline double App::ProfilerCPU::avg() const {
	return _avg;
}

inline double App::ProfilerCPU::minimum() const {
	return _min;
}

inline double App::ProfilerCPU::maximum() const {
	return _max;
}

inline float App::lifetimeInSeconds() const {
	return (_now - _initTime) / 1000.0f;
}

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

inline const std::string& App::appname() const {
	return _appname;
}

}

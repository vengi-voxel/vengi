/**
 * @file
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace io {

enum IOState {
	IOSTATE_LOADING, IOSTATE_LOADED, IOSTATE_FAILED
};

class IOResource {
protected:
	std::atomic<IOState> _state;

	IOResource() :
			_state(IOSTATE_LOADING) {
	}

	virtual ~IOResource() {
	}

public:
	inline bool isLoaded() const {
		return _state == io::IOSTATE_LOADED;
	}

	inline bool isFailed() const {
		return _state == io::IOSTATE_FAILED;
	}

	inline bool isLoading() const {
		return _state == io::IOSTATE_LOADING;
	}
};

}

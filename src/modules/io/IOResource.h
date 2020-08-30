/**
 * @file
 */

#pragma once

#include <atomic>

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

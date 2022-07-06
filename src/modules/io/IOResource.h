/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"

namespace io {

enum IOState {
	IOSTATE_LOADING, IOSTATE_LOADED, IOSTATE_FAILED
};

class IOResource {
protected:
	core::AtomicInt _state;

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

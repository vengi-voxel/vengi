/**
 * @file
 */

#pragma once

#include "Renderer.h"

namespace video {

class ScopedState {
private:
	const video::State _state;
	bool _old;
	const bool _enable;
public:
	ScopedState(video::State state, bool enable = true) :
			_state(state), _enable(enable) {
		if (_enable) {
			_old = video::enable(_state);
		} else {
			_old = video::disable(_state);
		}
	}

	~ScopedState() {
		if (_enable) {
			if (!_old) {
				video::disable(_state);
			}
		} else {
			if (_old) {
				video::enable(_state);
			}
		}
	}
};

}

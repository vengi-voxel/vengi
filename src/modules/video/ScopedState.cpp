/**
 * @file
 */

#include "ScopedState.h"
#include "Renderer.h"

namespace video {

ScopedState::ScopedState(video::State state, bool enable) :
		_state(state), _enable(enable) {
	if (_enable) {
		_old = video::enable(_state);
	} else {
		_old = video::disable(_state);
	}
}

ScopedState::~ScopedState() {
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

}

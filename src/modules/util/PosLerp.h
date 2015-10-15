#pragma once

#include "core/Log.h"
#include <glm/glm.hpp>

namespace util {

class PosLerp {
private:
	long MOVETIME = 200L;
	glm::vec3 _currentPosition;
	glm::vec3 _lastPosition;
	glm::vec3 _nextPosition;
	long _nextPosTime;
public:
	PosLerp() :
			_currentPosition(0.0f), _lastPosition(0.0f), _nextPosition(0.0f), _nextPosTime(0L) {
	}

	inline const glm::vec3& position() const {
		return _currentPosition;
	}

	inline void setPosition(long now, const glm::vec3& position) {
		_lastPosition = position;
		_currentPosition = position;
		_nextPosition = position;
		_nextPosTime = now;
	}

	inline void lerpPosition(long now, const glm::vec3& position) {
		_lastPosition = _currentPosition;
		_nextPosition = position;
		_nextPosTime = now + MOVETIME;
		Log::trace("update: c(%f:%f:%f) n(%f:%f:%f) l(%f:%f:%f)", _currentPosition.x, _currentPosition.y, _currentPosition.z,
				_nextPosition.x, _nextPosition.y, _nextPosition.z, _lastPosition.x, _lastPosition.y, _lastPosition.z);
	}

	void update(long now) {
		if (now < _nextPosTime) {
			const long remaining = _nextPosTime - now;
			const long passed = MOVETIME - remaining;
			const float lerp = passed / (float) MOVETIME;
			_currentPosition = _lastPosition + ((_nextPosition - _lastPosition) * lerp);
			Log::trace("lerp: %f, passed: %li c(%f:%f:%f) n(%f:%f:%f) l(%f:%f:%f)", lerp, passed, _currentPosition.x, _currentPosition.y,
					_currentPosition.z, _nextPosition.x, _nextPosition.y, _nextPosition.z, _lastPosition.x, _lastPosition.y,
					_lastPosition.z);
		} else {
			_currentPosition = _nextPosition;
		}
	}
};

}

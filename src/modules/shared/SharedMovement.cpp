/**
 * @file
 */

#include "SharedMovement.h"
#include "voxel/Constants.h"

namespace shared {

glm::vec3 SharedMovement::calculateDelta(const glm::quat& rot, float speed) {
	if (_gliding || _jumping) {
		glm::vec3 delta(0.0f);
		if (forward()) {
			delta += rot * (glm::forward * speed);
		} else if (backward()) {
			// you can only reduce speed - but not walk backward
			delta += rot * (glm::forward * speed / 10.0f);
		}
		return delta;
	}
	glm::vec3 delta(0.0f);
	if (left()) {
		delta += rot * (glm::left * speed);
	} else if (right()) {
		delta += rot * (glm::right * speed);
	}
	if (forward()) {
		delta += rot * (glm::forward * speed);
	} else if (backward()) {
		delta += rot * (glm::backward * speed);
	}
	return delta;
}

glm::vec3 SharedMovement::update(float deltaFrameSeconds, float orientation, float speed, const glm::vec3& currentPos, std::function<int(const glm::vec3& pos)> heightResolver) {
	glm::vec3 newPos = currentPos;
	if (deltaFrameSeconds > glm::epsilon<float>()) {
		const glm::quat& rot = glm::angleAxis(orientation, glm::up);
		speed *= deltaFrameSeconds;
		newPos += calculateDelta(rot, speed);
	}
	_groundHeight = heightResolver(newPos);
	if (_groundHeight < voxel::MIN_HEIGHT) {
		_groundHeight = voxel::MIN_HEIGHT;
	}
	_delay -= deltaFrameSeconds;
	const float inputDelaySeconds = 0.5f;
	if (jump()) {
		if (_gliding) {
			if (_delay <= 0.0f) {
				_gliding = false;
				_jumping = true;
				_delay = inputDelaySeconds;
			}
		} else if (_jumping) {
			if (_delay <= 0.0f) {
				_jumping = false;
				_gliding = true;
				_delay = inputDelaySeconds;
			}
		} else {
			_velocityY = 10.0f;
			_jumping = true;
			_delay = inputDelaySeconds;
		}
	}
	const float gravity = _gliding ? 0.1f : 20.0f;
	_velocityY -= gravity * deltaFrameSeconds;
	newPos.y += _velocityY * deltaFrameSeconds;
	if (newPos.y <= _groundHeight) {
		newPos.y = _groundHeight;
		_velocityY = 0.0f;
		_jumping = false;
		_gliding = false;
		_delay = 0.0f;
	}
	return newPos;
}

network::Animation SharedMovement::animation() const {
	if (_jumping) {
		return network::Animation::JUMP;
	}
	if (_gliding) {
		return network::Animation::GLIDE;
	}
	if (moving()) {
		return network::Animation::RUN;
	}
	return network::Animation::IDLE;
}

}

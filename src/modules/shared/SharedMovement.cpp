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

glm::vec3 SharedMovement::moveDelta(float speed, float orientation) {
	if (_deltaSeconds <= glm::epsilon<float>()) {
		return glm::zero<glm::vec3>();
	}

	const glm::quat& rot = glm::angleAxis(orientation, glm::up);
	speed *= _deltaSeconds;
	const glm::vec3& delta = calculateDelta(rot, speed);
	_deltaSeconds = 0.0f;
	return delta;
}

glm::vec3 SharedMovement::update(float deltaFrameSeconds, float orientation, double speed, const glm::vec3& currentPos, std::function<int(const glm::vec3& pos)> heightResolver) {
	_deltaSeconds = deltaFrameSeconds;

	const glm::vec3& md = moveDelta(speed, orientation);

	glm::vec3 newPos = currentPos + md;
	_groundHeight = heightResolver(newPos);
	if (_groundHeight < voxel::MIN_HEIGHT) {
		_groundHeight = voxel::MIN_HEIGHT;
	}
	_delay -= deltaFrameSeconds;
	if (jump()) {
		if (_gliding) {
			if (_delay <= 0.0f) {
				_gliding = false;
				_jumping = true;
				_delay = 0.5f;
			}
		} else if (_jumping) {
			if (_delay <= 0.0f) {
				_jumping = false;
				_gliding = true;
				_delay = 0.5f;
			}
		} else {
			_velocityY = 10.0f;
			_jumping = true;
			_delay = 0.5f;
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

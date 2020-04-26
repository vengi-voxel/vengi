/**
 * @file
 */

#include "SharedMovement.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "voxel/Constants.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace shared {

glm::vec3 SharedMovement::calculateDelta(const glm::quat& rot) {
	if (_jumping || _swimming) {
		glm::vec3 delta(0.0f);
		if (_swimming) {
			_speed *= 0.2f;
		}
		if (forward()) {
			delta += rot * (glm::forward * _speed);
		} else if (backward()) {
			// you can only reduce speed - but not walk backward
			delta += rot * (glm::forward * _speed / 10.0f);
		}
		return delta;
	} else if (_gliding) {
		// TODO: apply a speed debuff via attributes
		glm::vec3 delta(0.0f);
		if (!forward()) {
			_speed *= 0.2f;
		}
		delta += rot * (glm::forward * _speed);
		return delta;
	}
	glm::vec3 delta(0.0f);
	if (left()) {
		delta += rot * (glm::left * _speed);
	} else if (right()) {
		delta += rot * (glm::right * _speed);
	}
	if (forward()) {
		delta += rot * (glm::forward * _speed);
	} else if (backward()) {
		delta += rot * (glm::backward * _speed);
	}
	return delta;
}

float SharedMovement::gravity() const {
	if (_gliding) {
		return 0.1f;
	}
	if (_swimming) {
		if (forward() || backward()) {
			return -2.0f;
		}
		return 2.0f;
	}
	return 20.0f;
}

glm::vec3 SharedMovement::update(float deltaFrameSeconds, float orientation, float speed, const glm::vec3& currentPos, const WalkableFloorResolver& heightResolver) {
	core_trace_scoped(UpdateSharedMovement);
	_speed = speed;
	glm::vec3 newPos = currentPos;
	if (deltaFrameSeconds > glm::epsilon<float>()) {
		const glm::quat& rot = glm::angleAxis(orientation, glm::up);
		_speed *= deltaFrameSeconds;
		newPos += calculateDelta(rot);
	}
	const int maxWalkableHeight = 3;
	_groundHeight = heightResolver(glm::ivec3(glm::floor(newPos)), maxWalkableHeight);
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
				_fallingVelocity = 0.0f;
				_delay = inputDelaySeconds;
			}
		} else {
			_fallingVelocity = 10.0f;
			_jumping = true;
			_delay = inputDelaySeconds;
		}
	}
	if (_gliding) {
		_fallingVelocity = -gravity();
	} else {
		_fallingVelocity -= gravity() * deltaFrameSeconds;
	}
	newPos.y += _fallingVelocity * deltaFrameSeconds;
	if (newPos.y <= _groundHeight) {
		newPos.y = _groundHeight;
		_fallingVelocity = 0.0f;
		_jumping = false;
		_gliding = false;
		_delay = 0.0f;
	}

	if (newPos.y < voxel::MAX_WATER_HEIGHT) {
		_fallingVelocity = 0.0f;
		_jumping = false;
		_gliding = false;
		_delay = 0.0f;
		_swimming = true;
		_fallingVelocity = -2.0f;
	} else {
		_swimming = false;
	}
	return newPos;
}

network::Animation SharedMovement::animation() const {
	if (_swimming) {
		if (backward() || forward()) {
			return network::Animation::SWIM;
		}
		return network::Animation::IDLE;
	}
	if (_gliding) {
		return network::Animation::GLIDE;
	}
	if (_jumping) {
		return network::Animation::JUMP;
	}
	if (moving()) {
		return network::Animation::RUN;
	}
	return network::Animation::IDLE;
}

}

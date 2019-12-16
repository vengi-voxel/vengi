/**
 * @file
 */

#include "PlayerMovement.h"
#include "core/command/Command.h"
#include "voxel/Constants.h"

namespace frontend {

void PlayerMovement::construct() {
	Super::construct();
	core::Command::registerActionButton("jump", _jump);
}

void PlayerMovement::shutdown() {
	Super::shutdown();
	core::Command::unregisterActionButton("jump");
	_jump.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void PlayerMovement::updatePos(float orientation, float deltaFrameSeconds, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver) {
	const attrib::ShadowAttributes& attribs = entity->attrib();
	const double speed = attribs.current(attrib::Type::SPEED);
	const glm::vec3& md = moveDelta(speed, orientation);
	const glm::vec3& currentPos = entity->position();

	glm::vec3 newPos = currentPos + md;
	_groundHeight = heightResolver(newPos);
	if (_groundHeight < voxel::MIN_HEIGHT) {
		_groundHeight = voxel::MIN_HEIGHT;
	}
	_delay -= deltaFrameSeconds;
	if (jump()) {
		if (_flying) {
			if (_delay <= 0.0f) {
				_flying = false;
				_jumping = true;
				_delay = 0.5f;
			}
		} else if (_jumping) {
			if (_delay <= 0.0f) {
				_jumping = false;
				_flying = true;
				_delay = 0.5f;
			}
		} else {
			_velocityY = 10.0f;
			_jumping = true;
			_delay = 0.5f;
		}
	}
	const float gravity = _flying ? 0.1f : 20.0f;
	_velocityY -= gravity * deltaFrameSeconds;
	newPos.y += _velocityY * deltaFrameSeconds;
	if (newPos.y <= _groundHeight) {
		newPos.y = _groundHeight;
		_velocityY = 0.0f;
		_jumping = false;
		_flying = false;
		_delay = 0.0f;
	}
	if (_jumping) {
		entity->setAnimation(animation::Animation::Jump);
	} else if (_flying) {
		entity->setAnimation(animation::Animation::Glide);
	} else if (moving()) {
		entity->setAnimation(animation::Animation::Run);
	} else {
		entity->setAnimation(animation::Animation::Idle);
	}
	entity->setPosition(newPos);
}

glm::vec3 PlayerMovement::calculateDelta(const glm::quat& rot, float speed) {
	if (_flying || _jumping) {
		glm::vec3 delta(0.0f);
		if (forward()) {
			delta += rot * (glm::forward * speed);
		} else if (backward()) {
			// you can only reduce speed - but not walk backward
			delta += rot * (glm::forward * speed / 10.0f);
		}
		return delta;
	}
	return Movement::calculateDelta(rot, speed);
};

}

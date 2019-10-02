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

void PlayerMovement::updatePos(video::Camera& camera, float deltaFrameSeconds, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver) {
	const glm::quat& q = camera.quaternion();
	const double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
	const double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
	const double yaw = -atan2(siny_cosp, cosy_cosp);
	entity->setOrientation(yaw);

	const attrib::ShadowAttributes& attribs = entity->attrib();
	const double speed = attribs.current(attrib::Type::SPEED);
	const glm::vec3& md = moveDelta(speed, yaw);
	const glm::vec3& currentPos = entity->position();

	glm::vec3 newPos = currentPos + md;
	_groundHeight = heightResolver(newPos);
	if (_groundHeight < voxel::MIN_HEIGHT) {
		newPos = currentPos;
	}
	if (jump()) {
		if (_velocityY <= 0.0f) {
			if (_flying) {
				_flying = false;
				_jumping = true;
			} else if (_jumping) {
				_jumping = false;
				_flying = true;
			} else {
				_velocityY = 10.0f;
				_jumping = true;
			}
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

	camera.setTarget(newPos);
}

glm::vec3 PlayerMovement::calculateDelta(const glm::quat& rot, float speed) {
	glm::vec3 delta = Super::calculateDelta(rot, speed);
	if (jump()) {
		delta += glm::up;
	}
	return delta;
};

}

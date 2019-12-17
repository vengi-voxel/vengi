/**
 * @file
 */

#include "PlayerMovement.h"
#include "core/command/Command.h"

namespace frontend {

void PlayerMovement::construct() {
	core::Command::registerActionButton("jump", _jumpButton);
	core::Command::registerActionButton("move_forward", _moveForward);
	core::Command::registerActionButton("move_backward", _moveBackward);
	core::Command::registerActionButton("move_left", _moveLeft);
	core::Command::registerActionButton("move_right", _moveRight);
}

bool PlayerMovement::init() {
	return true;
}

void PlayerMovement::shutdown() {
	core::Command::unregisterActionButton("jump");
	core::Command::unregisterActionButton("move_forward");
	core::Command::unregisterActionButton("move_backward");
	core::Command::unregisterActionButton("move_left");
	core::Command::unregisterActionButton("move_right");
	_jumpButton.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveLeft.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void PlayerMovement::update(float deltaFrameSeconds, float orientation, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver) {
	const attrib::ShadowAttributes& attribs = entity->attrib();
	const double speed = attribs.current(attrib::Type::SPEED);
	const glm::vec3& currentPos = entity->position();
	if (_moveLeft.pressed()) {
		_move |= network::MoveDirection::MOVELEFT;
	} else {
		_move &= ~network::MoveDirection::MOVELEFT;
	}
	if (_moveRight.pressed()) {
		_move |= network::MoveDirection::MOVERIGHT;
	} else {
		_move &= ~network::MoveDirection::MOVERIGHT;
	}
	if (_moveForward.pressed()) {
		_move |= network::MoveDirection::MOVEFORWARD;
	} else {
		_move &= ~network::MoveDirection::MOVEFORWARD;
	}
	if (_moveBackward.pressed()) {
		_move |= network::MoveDirection::MOVEBACKWARD;
	} else {
		_move &= ~network::MoveDirection::MOVEBACKWARD;
	}
	if (_jumpButton.pressed()) {
		_move |= network::MoveDirection::JUMP;
	} else {
		_move &= ~network::MoveDirection::JUMP;
	}
	const glm::vec3& newPos = Super::update(deltaFrameSeconds, orientation, (float)speed, currentPos, heightResolver);
	entity->setPosition(newPos);
	entity->setAnimation(animation());
}

}

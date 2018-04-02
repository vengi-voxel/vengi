/**
 * @file
 */

#include "Movement.h"
#include "core/command/Command.h"
#include "core/GLM.h"

namespace frontend {

bool Movement::init() {
	return true;
}

void Movement::onConstruct() {
	core::Command::registerActionButton("move_forward", _moveForward);
	core::Command::registerActionButton("move_backward", _moveBackward);
	core::Command::registerActionButton("move_left", _moveLeft);
	core::Command::registerActionButton("move_right", _moveRight);
}

void Movement::shutdown() {
	_millis = 0ul;
	core::Command::unregisterActionButton("move_forward");
	core::Command::unregisterActionButton("move_backward");
	core::Command::unregisterActionButton("move_left");
	core::Command::unregisterActionButton("move_right");
	_moveLeft.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void Movement::update(uint64_t deltaMillis) {
	_millis += deltaMillis;
}

// TODO: add jumping
glm::vec3 Movement::moveDelta(float speed) {
	glm::vec3 delta(0.0f);
	if (_millis <= 0) {
		return delta;
	}
	speed *= static_cast<float>(_millis);
	if (_moveLeft.pressed()) {
		delta += glm::left * speed;
	} else if (_moveRight.pressed()) {
		delta += glm::right * speed;
	}
	if (_moveForward.pressed()) {
		delta += glm::forward * speed;
	} else if (_moveBackward.pressed()) {
		delta += glm::backward * speed;
	}
	_millis = 0ul;
	return delta;
}

}

/**
 * @file
 */

#include "ActionButton.h"

namespace core {

ActionButton::ActionButton() {
	for (int i = 0; i < ACTION_BUTTON_KEY_AMOUNT; ++i) {
		pressedKeys[i] = ACTION_BUTTON_NO_KEY;
	}
}

bool ActionButton::execute(uint64_t now, uint64_t delayBetweenExecutions, std::function<void()> executionCallback) {
	if (now - lastPressed < delayBetweenExecutions) {
		return false;
	}
	executionCallback();
	lastPressed = now;
	return true;
}

bool ActionButton::handleDown(int32_t key, uint64_t millis) {
	if (this->bindingContext != BindingContext::All && this->bindingContext != core::bindingContext()) {
		return false;
	}
	for (int i = 0; i < ACTION_BUTTON_KEY_AMOUNT; ++i) {
		if (key == pressedKeys[i]) {
			return false;
		}
	}
	const bool alreadyDown = pressed();
	for (int i = 0; i < ACTION_BUTTON_KEY_AMOUNT; ++i) {
		if (pressedKeys[i] != ACTION_BUTTON_NO_KEY) {
			continue;
		}
		pressedKeys[i] = key;
		if (!alreadyDown) {
			pressedMillis = millis;
		}
		return true;
	}
	return false;
}

bool ActionButton::handleUp(int32_t key, uint64_t releasedMillis) {
	if (key == ACTION_BUTTON_ALL_KEYS) {
		for (int i = 0; i < ACTION_BUTTON_KEY_AMOUNT; ++i) {
			pressedKeys[i] = ACTION_BUTTON_NO_KEY;
		}
		return true;
	}
	for (int i = 0; i < ACTION_BUTTON_KEY_AMOUNT; ++i) {
		if (pressedKeys[i] != key) {
			continue;
		}
		pressedKeys[i] = ACTION_BUTTON_NO_KEY;
		if (!pressed()) {
			durationMillis = releasedMillis - pressedMillis;
			return true;
		}
		break;
	}
	return false;
}

}

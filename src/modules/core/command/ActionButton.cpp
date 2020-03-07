/**
 * @file
 */

#include "ActionButton.h"

namespace core {

ActionButton::ActionButton() {
	for (int & pressedKey : pressedKeys) {
		pressedKey = ACTION_BUTTON_NO_KEY;
	}
}

bool ActionButton::execute(uint64_t now, uint64_t delayBetweenExecutions, const std::function<void()>& executionCallback) {
	if (now - lastPressed < delayBetweenExecutions) {
		return false;
	}
	executionCallback();
	lastPressed = now;
	return true;
}

bool ActionButton::handleDown(int32_t key, uint64_t millis) {
	for (int pressedKey : pressedKeys) {
		if (key == pressedKey) {
			return false;
		}
	}
	const bool alreadyDown = pressed();
	for (int & pressedKey : pressedKeys) {
		if (pressedKey != ACTION_BUTTON_NO_KEY) {
			continue;
		}
		pressedKey = key;
		if (!alreadyDown) {
			pressedMillis = millis;
		}
		return true;
	}
	return false;
}

bool ActionButton::handleUp(int32_t key, uint64_t releasedMillis) {
	if (key == ACTION_BUTTON_ALL_KEYS) {
		for (int & pressedKey : pressedKeys) {
			pressedKey = ACTION_BUTTON_NO_KEY;
		}
		return true;
	}
	for (int & pressedKey : pressedKeys) {
		if (pressedKey != key) {
			continue;
		}
		pressedKey = ACTION_BUTTON_NO_KEY;
		if (!pressed()) {
			durationMillis = (uint32_t)(releasedMillis - pressedMillis);
			return true;
		}
		break;
	}
	return false;
}

}

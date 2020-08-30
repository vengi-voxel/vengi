/**
 * @file
 */

#include "ActionButton.h"

namespace command {

ActionButton::ActionButton() {
	for (int & pressedKey : pressedKeys) {
		pressedKey = ACTION_BUTTON_NO_KEY;
	}
}

bool ActionButton::execute(double nowSeconds, double delayBetweenExecutions, const std::function<void()>& executionCallback) {
	if (nowSeconds - lastPressed < delayBetweenExecutions) {
		return false;
	}
	executionCallback();
	lastPressed = nowSeconds;
	return true;
}

bool ActionButton::handleDown(int32_t key, double pressedSeconds) {
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
			this->pressedSeconds = pressedSeconds;
		}
		return true;
	}
	return false;
}

bool ActionButton::handleUp(int32_t key, double releasedSeconds) {
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
			durationSeconds = releasedSeconds - pressedSeconds;
			return true;
		}
		break;
	}
	return false;
}

}

/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <functional>
#include "core/BindingContext.h"

namespace core {

static constexpr int ACTION_BUTTON_KEY_AMOUNT = 10;
static constexpr int ACTION_BUTTON_NO_KEY = 0;
static constexpr int ACTION_BUTTON_ALL_KEYS = -1;

/**
 * @brief Action buttons can be bound to multiple keys and calculate the
 * milliseconds they were triggered. They can be bound to console commands
 * that itself can be bound to keys.
 *
 * @see core::Command::registerActionButton()
 */
struct ActionButton {
	/**
	 * A list of all keys that triggered this action button
	 */
	int32_t pressedKeys[ACTION_BUTTON_KEY_AMOUNT]{};
	/**
	 * The timestamp at which the action button was initially triggered
	 */
	double pressedSeconds = 0.0;
	/**
	 * After all triggering keys were released, the duration is calculated
	 */
	double durationSeconds = 0.0;

	double lastPressed = 0.0;

	ActionButton();
	virtual ~ActionButton() {}

	/**
	 * @return @c true if any of the bound keys is still active
	 */
	bool pressed() const;

	bool execute(double nowSeconds, double delayBetweenExecutions, const std::function<void()>& executionCallback);

	/**
	 * @return @c true if the action button was initially triggered
	 */
	virtual bool handleDown(int32_t key, double pressedSeconds);
	/**
	 * @param[in] key If @c -1 is given here, everything is released
	 * @return @c true if the action button was completely released
	 */
	virtual bool handleUp(int32_t key, double releasedSeconds);
};

inline bool ActionButton::pressed() const {
	for (int i = 0; i < ACTION_BUTTON_KEY_AMOUNT; ++i) {
		if (pressedKeys[i]) {
			return true;
		}
	}
	return false;
}

}

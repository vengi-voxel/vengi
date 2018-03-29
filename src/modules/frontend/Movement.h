/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
#include <glm/vec3.hpp>
#include <stdint.h>

namespace frontend {

/**
 * @brief Movement component that does the input listening
 *
 * @see core::ActionButton
 */
class Movement {
private:
	core::ActionButton _moveLeft;
	core::ActionButton _moveRight;
	core::ActionButton _moveBackward;
	core::ActionButton _moveForward;

	uint64_t _millis = 0ul;

public:
	void onConstruct();
	bool init();
	void update(uint64_t deltaMillis);
	void shutdown();

	bool left() const;
	bool right() const;
	bool forward() const;
	bool backward() const;

	/**
	 * @note update() must have been called with proper delta milliseconds.
	 */
	glm::vec3 moveDelta(float speed);
};

inline bool Movement::left() const {
	return _moveLeft.pressed();
}

inline bool Movement::right() const {
	return _moveRight.pressed();
}

inline bool Movement::forward() const {
	return _moveForward.pressed();
}

inline bool Movement::backward() const {
	return _moveBackward.pressed();
}

}

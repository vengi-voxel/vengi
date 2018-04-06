/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
#include "core/IComponent.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <stdint.h>

namespace frontend {

/**
 * @brief Movement component that does the input listening
 *
 * @see core::ActionButton
 */
class Movement : public core::IComponent {
private:
	core::ActionButton _moveLeft;
	core::ActionButton _moveRight;
	core::ActionButton _moveBackward;
	core::ActionButton _moveForward;

	uint64_t _millis = 0ul;

public:
	void construct() override;
	bool init() override;
	void update(uint64_t deltaMillis);
	void shutdown() override;

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

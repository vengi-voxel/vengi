/**
 * @file
 */

#pragma once

#include "command/ActionButton.h"
#include "core/DeltaFrameSeconds.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace util {

/**
 * @brief Movement component that does the input listening
 *
 * @see command::ActionButton
 */
class Movement : public core::DeltaFrameSeconds {
protected:
	command::ActionButton _moveLeft;
	command::ActionButton _moveRight;
	command::ActionButton _moveBackward;
	command::ActionButton _moveForward;
	command::ActionButton _jump;

	glm::vec3 calculateDelta(double speed) const;

public:
	virtual ~Movement() {
	}
	void construct() override;
	bool init() override;
	void update(double nowSeconds);
	void shutdown() override;

	bool left() const;
	bool right() const;
	bool forward() const;
	bool backward() const;
	bool jump() const;

	bool moving() const;

	/**
	 * @brief Calculate the movement without rotation in @c glm::forward(), @c glm::up(), @c glm::right() direction
	 * @param speed The speed to move in the given direction
	 * @note update() must have been called with proper delta milliseconds.
	 */
	glm::vec3 moveDelta(double speed) const;
};

inline bool Movement::moving() const {
	return left() || right() || forward() || backward();
}

inline bool Movement::jump() const {
	return _jump.pressed();
}

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

} // namespace util

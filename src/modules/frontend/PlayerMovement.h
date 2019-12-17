/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
#include "core/IComponent.h"
#include "frontend/ClientEntity.h"
#include "video/Camera.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <stdint.h>
#include <functional>

namespace frontend {

/**
 * @brief Movement component that does the input listening
 *
 * @see core::ActionButton
 */
class PlayerMovement : public core::IComponent {
private:
	core::ActionButton _moveLeft;
	core::ActionButton _moveRight;
	core::ActionButton _moveBackward;
	core::ActionButton _moveForward;
	core::ActionButton _jump;

	bool _jumping = false;
	bool _gliding = false;
	float _velocityY = 0.0f;
	int _groundHeight = 0;
	float _delay = 0.0f;

	uint64_t _deltaMillis = 0ul;

	glm::vec3 calculateDelta(const glm::quat& rot, float speed);
	/**
	 * @note update() must have been called with proper delta milliseconds.
	 */
	glm::vec3 moveDelta(float speed, float orientation);
public:
	virtual ~PlayerMovement() {}

	bool init() override;
	void update(uint64_t deltaMillis);
	void construct() override;
	void shutdown() override;

	bool left() const;
	bool right() const;
	bool forward() const;
	bool backward() const;
	bool jump() const;

	bool moving() const;

	void updatePos(float orientation, float deltaFrameSeconds, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver);
	/**
	 * @brief Available after updatePos() was called.
	 */
	int groundHeight() const;
};

inline int PlayerMovement::groundHeight() const {
	return _groundHeight;
}

inline bool PlayerMovement::jump() const {
	return _jump.pressed();
}

inline bool PlayerMovement::moving() const {
	return left() || right() || forward() || backward();
}

inline bool PlayerMovement::left() const {
	return _moveLeft.pressed();
}

inline bool PlayerMovement::right() const {
	return _moveRight.pressed();
}

inline bool PlayerMovement::forward() const {
	return _moveForward.pressed();
}

inline bool PlayerMovement::backward() const {
	return _moveBackward.pressed();
}

}

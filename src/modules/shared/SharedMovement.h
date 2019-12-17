/**
 * @file
 */

#include "core/GLM.h"
#include "Shared_generated.h"
#include <stdint.h>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <functional>

namespace shared {

class SharedMovement {
protected:
	network::MoveDirection _move = network::MoveDirection::NONE;
	bool _gliding = false;
	bool _jumping = false;

	float _fallingVelocity = 0.0f;
	int _groundHeight = 0;
	float _delay = 0.0f;

	glm::vec3 calculateDelta(const glm::quat& rot, float speed);
public:
	glm::vec3 update(float deltaFrameSeconds, float orientation, float speed, const glm::vec3& currentPos, std::function<int(const glm::vec3& pos)> heightResolver);

	bool left() const;
	bool right() const;
	bool forward() const;
	bool backward() const;
	bool jump() const;
	bool gliding() const;

	bool moving() const;
	network::Animation animation() const;

	int groundHeight() const;
};

inline bool SharedMovement::moving() const {
	return left() || right() || forward() || backward();
}

inline bool SharedMovement::left() const {
	return (_move & network::MoveDirection::MOVELEFT) == network::MoveDirection::MOVELEFT;
}

inline bool SharedMovement::right() const {
	return (_move & network::MoveDirection::MOVERIGHT) == network::MoveDirection::MOVERIGHT;
}

inline bool SharedMovement::forward() const {
	return (_move & network::MoveDirection::MOVEFORWARD) == network::MoveDirection::MOVEFORWARD;
}

inline bool SharedMovement::backward() const {
	return (_move & network::MoveDirection::MOVEBACKWARD) == network::MoveDirection::MOVEBACKWARD;
}

inline bool SharedMovement::jump() const {
	return (_move & network::MoveDirection::JUMP) == network::MoveDirection::JUMP;
}

inline bool SharedMovement::gliding() const {
	return _gliding;
}

inline int SharedMovement::groundHeight() const {
	return _groundHeight;
}

}

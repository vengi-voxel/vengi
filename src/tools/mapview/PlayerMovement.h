/**
 * @file
 */

#pragma once

#include "frontend/Movement.h"
#include "frontend/ClientEntity.h"
#include "video/Camera.h"
#include <functional>

namespace frontend {

class PlayerMovement : public frontend::Movement {
private:
	using Super = Movement;
	core::ActionButton _jump;

	bool _jumping = false;
	bool _flying = false;
	float _velocityY = 0.0f;
	int _groundHeight = 0;

	glm::vec3 calculateDelta(const glm::quat& rot, float speed) override;
public:
	bool jump() const;

	void updatePos(video::Camera& camera, float deltaFrameSeconds, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver);

	/**
	 * @brief Available after updatePos() was called.
	 */
	int groundHeight() const;

	void construct() override;
	void shutdown() override;
};

inline int PlayerMovement::groundHeight() const {
	return _groundHeight;
}

inline bool PlayerMovement::jump() const {
	return _jump.pressed();
}

}

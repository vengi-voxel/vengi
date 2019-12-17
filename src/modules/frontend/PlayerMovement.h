/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
#include "core/IComponent.h"
#include "frontend/ClientEntity.h"
#include "video/Camera.h"
#include "shared/SharedMovement.h"


namespace frontend {

/**
 * @brief Movement component that does the input listening
 *
 * @see core::ActionButton
 */
class PlayerMovement : public core::IComponent, public shared::SharedMovement {
private:
	using Super = shared::SharedMovement;
	core::ActionButton _moveLeft;
	core::ActionButton _moveRight;
	core::ActionButton _moveBackward;
	core::ActionButton _moveForward;
	core::ActionButton _jumpButton;

public:
	bool init() override;
	void update(float deltaFrameSeconds, float orientation, ClientEntityPtr& entity, std::function<int(const glm::vec3& pos)> heightResolver);
	void construct() override;
	void shutdown() override;
};

}

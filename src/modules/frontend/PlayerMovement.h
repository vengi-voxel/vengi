/**
 * @file
 */

#pragma once

#include "core/command/ActionButton.h"
#include "core/IComponent.h"
#include "frontend/ClientEntity.h"
#include "video/Camera.h"
#include "shared/SharedMovement.h"
#include "audio/SoundManager.h"

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

	int _footstepSoundChannel = -1;
	int _ambienceSoundChannel = -1;

	audio::SoundManagerPtr _soundManager;

public:
	PlayerMovement(const audio::SoundManagerPtr& soundManager);
	bool init() override;
	void update(double deltaFrameSeconds, float orientation, ClientEntityPtr& entity, const shared::WalkableFloorResolver& heightResolver);
	void construct() override;
	void shutdown() override;
};

}

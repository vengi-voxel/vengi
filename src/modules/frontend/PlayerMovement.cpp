/**
 * @file
 */

#include "PlayerMovement.h"
#include "command/Command.h"
#include "core/Trace.h"

namespace frontend {

static const char* FootStepSoundStr[] = {
	nullptr,
	"footstep_glass",
	"water_move",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	"footstep_generic",
	nullptr,
	"footstep_generic",
	"footstep_generic",
	"footstep_generic"
};
static_assert(lengthof(FootStepSoundStr) == (int)voxel::VoxelType::Max, "footstep sound array size doesn't match the available voxel types");


PlayerMovement::PlayerMovement(const audio::SoundManagerPtr &soundManager) : _soundManager(soundManager) {
}

void PlayerMovement::construct() {
	command::Command::registerActionButton("jump", _jumpButton);
	command::Command::registerActionButton("move_forward", _moveForward);
	command::Command::registerActionButton("move_backward", _moveBackward);
	command::Command::registerActionButton("move_left", _moveLeft);
	command::Command::registerActionButton("move_right", _moveRight);
}

bool PlayerMovement::init() {
	return true;
}

void PlayerMovement::shutdown() {
	command::Command::unregisterActionButton("jump");
	command::Command::unregisterActionButton("move_forward");
	command::Command::unregisterActionButton("move_backward");
	command::Command::unregisterActionButton("move_left");
	command::Command::unregisterActionButton("move_right");
	_jumpButton.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveLeft.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveRight.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveForward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
	_moveBackward.handleUp(command::ACTION_BUTTON_ALL_KEYS, 0ul);
}

void PlayerMovement::update(double deltaFrameSeconds, float orientation, ClientEntityPtr& entity, const shared::WalkableFloorResolver& heightResolver) {
	core_trace_scoped(UpdateMovement);
	const attrib::ShadowAttributes& attribs = entity->attrib();
	const double speed = attribs.current(attrib::Type::SPEED);
	const glm::vec3& currentPos = entity->position();
	if (_moveLeft.pressed()) {
		_move |= network::MoveDirection::MOVELEFT;
	} else {
		_move &= ~network::MoveDirection::MOVELEFT;
	}
	if (_moveRight.pressed()) {
		_move |= network::MoveDirection::MOVERIGHT;
	} else {
		_move &= ~network::MoveDirection::MOVERIGHT;
	}
	if (_moveForward.pressed()) {
		_move |= network::MoveDirection::MOVEFORWARD;
	} else {
		_move &= ~network::MoveDirection::MOVEFORWARD;
	}
	if (_moveBackward.pressed()) {
		_move |= network::MoveDirection::MOVEBACKWARD;
	} else {
		_move &= ~network::MoveDirection::MOVEBACKWARD;
	}
	if (_jumpButton.pressed()) {
		_move |= network::MoveDirection::JUMP;
	} else {
		_move &= ~network::MoveDirection::JUMP;
	}
	const bool prevWaterState = _inWater;
	// TODO: https://www.gabrielgambetta.com/client-side-prediction-server-reconciliation.html
	const glm::vec3& newPos = Super::update(deltaFrameSeconds, orientation, speed, currentPos, heightResolver);

	const glm::vec3 windPos(newPos.x, voxel::MAX_HEIGHT, newPos.z);
	const int ambienceSoundChannel = _soundManager->play(_ambienceSoundChannel, "ambience_wind", windPos, true);
	if (ambienceSoundChannel != -1) {
		_ambienceSoundChannel = ambienceSoundChannel;
		_soundManager->volume(ambienceSoundChannel, 32);
	}

	int footstepChannel = _footstepSoundChannel;
	if (_inWater && !prevWaterState) {
		footstepChannel = _soundManager->play(footstepChannel, "water_enter", newPos, false);
	} else if (!_inWater && prevWaterState) {
		footstepChannel = _soundManager->play(footstepChannel, "water_leave", newPos, false);
	} else if (walking()) {
		const voxel::VoxelType material = groundVoxel().getMaterial();
		const char *sound = FootStepSoundStr[(int)material];
		if (sound != nullptr) {
			footstepChannel = _soundManager->play(footstepChannel, sound, newPos, false);
		}
	}

	if (footstepChannel != -1) {
		_footstepSoundChannel = footstepChannel;
	}

	entity->setOrientation(orientation);
	entity->setPosition(newPos);
	const animation::Animation anim = animation();
	entity->addAnimation(anim, 0.1);
}

}

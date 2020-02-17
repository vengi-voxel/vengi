/**
 * @file
 */

#include "PlayerAction.h"
#include "core/command/Command.h"
#include "frontend/ClientEntity.h"

namespace frontend {

network::Animation PlayerAction::animation() const {
	if (_triggerAction.pressed()) {
		return network::Animation::TOOL;
	}
	return network::Animation::IDLE;
}

bool PlayerAction::init() {
	return true;
}

void PlayerAction::update(const ClientEntityPtr& entity) {
	entity->setAnimation(animation());
}

void PlayerAction::construct() {
	core::Command::registerActionButton("triggeraction", _triggerAction);
}

void PlayerAction::shutdown() {
	core::Command::unregisterActionButton("triggeraction");
	_triggerAction.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

}

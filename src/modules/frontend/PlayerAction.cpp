/**
 * @file
 */

#include "PlayerAction.h"
#include "core/command/Command.h"
#include "frontend/ClientEntity.h"

namespace frontend {

bool PlayerAction::init() {
	return true;
}

void PlayerAction::update(const ClientEntityPtr& entity) {
	// TODO: if not gliding or diving
	if (_triggerAction.pressed()) {
		entity->addAnimation(network::Animation::TOOL, 0.1f);
	}
}

void PlayerAction::construct() {
	core::Command::registerActionButton("triggeraction", _triggerAction);
}

void PlayerAction::shutdown() {
	core::Command::unregisterActionButton("triggeraction");
	_triggerAction.handleUp(core::ACTION_BUTTON_ALL_KEYS, 0ul);
}

}

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

void PlayerAction::update(uint64_t now, const ClientEntityPtr& entity) {
	// TODO: if not gliding or diving
	if (_triggerAction.pressed()) {
		_triggerAction.execute(now, 100ul, [&entity, this] () {
			entity->addAnimation(network::Animation::TOOL, 0.1f);
			++_triggerActionCounter;
		});
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

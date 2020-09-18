/**
 * @file
 */

#include "CooldownHandler.h"
#include "shared/ProtocolEnum.h"
#include "core/Log.h"

namespace client {

void CooldownHandler::startCooldown(network::CooldownType id, uint64_t startMillis, uint64_t duration) {
	const char *name = network::toString(id, network::EnumNamesCooldownType());
	Log::info("Starting cooldown: %s", name);
}

void CooldownHandler::stopCooldown(network::CooldownType id) {
	const char *name = network::toString(id, network::EnumNamesCooldownType());
	Log::info("Stopping cooldown: %s", name);
}

}

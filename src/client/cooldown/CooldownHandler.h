/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "network/ProtocolEnum.h"
#include "core/Log.h"

namespace client {

/**
 * @brief This class is handling the server cooldown visualisation on the client side
 */
class CooldownHandler {
public:
	void startCooldown(network::CooldownType id, uint64_t startMillis, uint64_t duration) {
		const char *name = network::toString(id, network::EnumNamesCooldownType());
		Log::info("Starting cooldown: %s", name);
	}

	void stopCooldown(network::CooldownType id) {
		const char *name = network::toString(id, network::EnumNamesCooldownType());
		Log::info("Stopping cooldown: %s", name);
	}
};

}
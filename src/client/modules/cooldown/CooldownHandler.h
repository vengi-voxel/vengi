/**
 * @file
 */

#pragma once

#include "Shared_generated.h"

namespace client {

/**
 * @brief This class is handling the server cooldown visualisation on the client side
 */
class CooldownHandler {
public:
	void startCooldown(network::CooldownType id, uint64_t startMillis, uint64_t duration);

	void stopCooldown(network::CooldownType id);
};

}

/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(StopCooldown) {
	network::CooldownType id = message->id();
	client->cooldownHandler().stopCooldown(id);
}

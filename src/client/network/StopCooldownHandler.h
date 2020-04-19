/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(StopCooldown) {
	network::CooldownType id = message->id();
	const char *name = network::toString(id, EnumNamesCooldownType());
	Log::info("Stopping cooldown: %s", name);
}
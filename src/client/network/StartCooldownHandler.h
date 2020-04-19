/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(StartCooldown) {
	network::CooldownType id = message->id();
	const char *name = network::toString(id, EnumNamesCooldownType());
	Log::info("Starting cooldown: %s", name);
}
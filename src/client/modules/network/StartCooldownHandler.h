/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(StartCooldown) {
	network::CooldownType id = message->id();
	client->cooldownHandler().startCooldown(id, message->start_utc_millis(), message->duration_millis());
}

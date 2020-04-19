/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(StartCooldown) {
	network::CooldownType id = message->id();
	client->cooldownHandler().startCooldown(id, message->startUTCMillis(), message->durationMillis());
}

/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(EntityRemove) {
	const frontend::ClientEntityId id = message->id();
	client->entityRemove(id);
}

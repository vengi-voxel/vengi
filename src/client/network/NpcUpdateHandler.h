#pragma once

#include "IClientProtocolHandler.h"

/**
 * Handler that updates a particular entity on the client side
 */
CLIENTPROTOHANDLERIMPL(NpcUpdate) {
	const network::messages::Vec3 *pos = message->pos();
	const glm::vec3 npcPos(pos->x(), pos->y(), pos->z());
	client->npcUpdate(message->id(), npcPos, message->rotation());
}

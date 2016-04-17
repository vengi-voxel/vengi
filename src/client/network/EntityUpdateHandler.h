#pragma once

#include "IClientProtocolHandler.h"

/**
 * Updates your own client
 */
CLIENTPROTOHANDLERIMPL(EntityUpdate) {
	const network::messages::Vec3 *pos = message->pos();
	if (pos == nullptr)
		return;
	const glm::vec3 userPos(pos->x(), pos->y(), pos->z());
	client->entityUpdate(message->id(), userPos, message->rotation());
}

/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

/**
 * Updates your own client
 */
CLIENTPROTOHANDLERIMPL(EntityUpdate) {
	const network::messages::Vec3 *pos = message->pos();
	if (pos == nullptr) {
		return;
	}
	const glm::vec3 entityPos(pos->x(), pos->y(), pos->z());
	client->entityUpdate(message->id(), entityPos, message->rotation());
}

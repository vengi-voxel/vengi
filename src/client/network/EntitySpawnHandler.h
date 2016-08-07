/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

/**
 * Spawn handler that announces a new npc appearance to your client.
 */
CLIENTPROTOHANDLERIMPL(EntitySpawn) {
	const network::messages::Vec3 *pos = message->pos();
	client->entitySpawn(message->id(), message->type(), message->rotation(), glm::vec3(pos->x(), pos->y(), pos->z()));
}

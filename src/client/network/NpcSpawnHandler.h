/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

/**
 * Spawn handler that announces a new npc appearance to your client.
 */
CLIENTPROTOHANDLERIMPL(NpcSpawn) {
	const network::messages::Vec3 *pos = message->pos();
	client->npcSpawn(message->id(), message->type(), glm::vec3(pos->x(), pos->y(), pos->z()));
}

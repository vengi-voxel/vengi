/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"
#include "animation/Animation.h"

/**
 * Spawn handler that announces a new npc appearance to your client.
 */
CLIENTPROTOHANDLERIMPL(EntitySpawn) {
	const network::Vec3 *pos = message->pos();
	client->entitySpawn(message->id(), message->type(), message->rotation(), glm::vec3(pos->x(), pos->y(), pos->z()), (animation::Animation)message->animation());
}

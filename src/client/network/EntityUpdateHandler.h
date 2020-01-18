/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"
#include "animation/Animation.h"

/**
 * Updates @c frontend::ClientEntity instances identified by the given @c frontend::ClientEntityId
 */
CLIENTPROTOHANDLERIMPL(EntityUpdate) {
	const frontend::ClientEntityId id = message->id();
	const frontend::ClientEntityPtr& entity = client->getEntity(id);
	if (!entity) {
		return;
	}
	const network::Vec3 *_pos = message->pos();
	const network::Animation animation = message->animation();
	const glm::vec3 pos(_pos->x(), _pos->y(), _pos->z());
	const float orientation = message->rotation();
	entity->setPosition(pos);
	entity->setOrientation(orientation);
	entity->setAnimation(animation);
}

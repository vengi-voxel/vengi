#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(UserSpawn) {
	const network::messages::Vec3 *pos = message->pos();
	client->spawn(message->id(), message->name()->c_str(), glm::vec3(pos->x(), pos->y(), pos->z()));
}

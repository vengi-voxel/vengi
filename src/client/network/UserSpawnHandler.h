/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(UserSpawn) {
	const frontend::ClientEntityId id = message->id();
	const char* name = message->name()->c_str();
	const network::Vec3 *_pos = message->pos();
	const glm::vec3 pos(_pos->x(), _pos->y(), _pos->z());
	const float orientation = message->rotation();
	client->spawn(id, name, pos, orientation);
}

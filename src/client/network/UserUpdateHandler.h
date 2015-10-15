#pragma once

#include "IClientProtocolHandler.h"

/**
 * Updates your own client
 */
CLIENTPROTOHANDLERIMPL(UserUpdate) {
	const network::messages::Vec3 *pos = message->pos();
	if (pos == nullptr)
		return;
	client->userUpdate(glm::vec3(pos->x(), pos->y(), pos->z()));
}

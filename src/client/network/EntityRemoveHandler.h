#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(EntityRemove) {
	client->entityRemove(message->id());
}

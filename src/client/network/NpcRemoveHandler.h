#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(NpcRemove) {
	client->npcRemove(message->id());
}

/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "IUserProtocolHandler.h"

namespace backend {

USERPROTOHANDLERIMPL(Attack) {
	user->updateLastActionTime();
	user->attack(message->targetId());
}

}

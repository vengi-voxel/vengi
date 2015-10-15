#pragma once

#include "network/Network.h"
#include "IUserProtocolHandler.h"

namespace backend {

USERPROTOHANDLERIMPL(Move) {
	user->changeMovement(message->direction(), message->pitch(), message->yaw());
}

}

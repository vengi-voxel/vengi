#pragma once

#include "network/Network.h"
#include "IUserProtocolHandler.h"

namespace backend {

USERPROTOHANDLERIMPL(UserDisconnect) {
	user->disconnect();
}

}

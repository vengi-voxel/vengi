/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "IUserProtocolHandler.h"

namespace backend {

/**
 * A client is not directly disconnected if the game is closed. This would otherwise always lead to
 * a situation where you could escape a fight. Thus we trigger a cooldown and abort it once the player
 * was hit or got interacted with. If nothing happens for that player for quite some time, the cooldown
 * is triggered again...
 */
USERPROTOHANDLERIMPL(UserDisconnect) {
	user->logoutMgr().triggerLogout();
}

}

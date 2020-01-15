/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "IUserProtocolHandler.h"

namespace backend {

/**
 * Sent from the user after the initialization and connection was successful.
 * Marker that the user is now ready to start playing.
 */
USERPROTOHANDLERIMPL(UserConnected) {
}

}

/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

/**
 * Looks like the username password combination is wrong
 */
CLIENTPROTOHANDLERIMPL(AuthFailed) {
	client->authFailed();
}

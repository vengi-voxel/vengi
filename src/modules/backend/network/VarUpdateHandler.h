/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "IUserProtocolHandler.h"

namespace backend {

/**
 * If a user modifies a @c core::Var with the flag @c core::CV_BROADCAST we will get the
 * update here and make sure that every other client that sees the broadcasting client
 * will get an update
 */
USERPROTOHANDLERIMPL(VarUpdate) {
	// TODO: implement me
}

}

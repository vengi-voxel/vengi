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
	const auto *vars = message->vars();
	for (const auto& v : *vars) {
		const auto* name = v->name();
		const auto* value = v->value();
		user->userinfo(name->c_str(), value->c_str());
	}
	user->broadcastUserinfo();
}

}

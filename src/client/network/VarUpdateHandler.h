/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"
#include "animation/Animation.h"

/**
 * @brief Set the values from the server to your own cvars
 */
CLIENTPROTOHANDLERIMPL(VarUpdate) {
	for (const auto& v : *message->vars()) {
		auto *name = v->name();
		auto *value = v->value();
		core::Var::get(name->str(), "", core::CV_NOPERSIST | core::CV_REPLICATE)->setVal(value->str());
	}
}

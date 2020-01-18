/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(UserInfo) {
	const frontend::ClientEntityId id = message->id();
	const frontend::ClientEntityPtr& entity = client->getEntity(id);
	for (const auto& v : *message->vars()) {
		auto *name = v->name();
		auto *value = v->value();
		entity->userinfo(name->str(), value->str());
	}
}

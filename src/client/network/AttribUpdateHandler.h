/**
 * @file
 */

#pragma once

#include "IClientProtocolHandler.h"

CLIENTPROTOHANDLERIMPL(AttribUpdate) {
	const frontend::ClientEntityId id = message->id();
	const flatbuffers::Vector<flatbuffers::Offset<AttribEntry>> *attribs = message->attribs();
	const frontend::ClientEntityPtr& entity = client->getEntity(id);
	if (!entity) {
		Log::error("Got attribute update for unknown entity");
		return;
	}
	attrib::ShadowAttributes& attrib = entity->attrib();
	for (const AttribEntry* e : *attribs) {
		const char *typeStr = network::toString(e->type(), ::network::EnumNamesAttribType());
		if (e->current()) {
			Log::debug("Set current for %s to %f", typeStr, e->value());
			attrib.setCurrent(e->type(), e->value());
		} else {
			Log::debug("Set max for %s to %f", typeStr, e->value());
			attrib.setMax(e->type(), e->value());
		}
	}
}

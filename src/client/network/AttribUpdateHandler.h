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
		return;
	}
	attrib::Attributes& attrib = entity->attrib();
	for (const AttribEntry* e : *attribs) {
		attrib.setCurrent(e->type(), e->value());
	}
}

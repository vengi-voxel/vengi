/**
 * @file
 */
#pragma once

#include "ProtocolHandler.h"
#include "core/collection/Map.h"

namespace voxedit {

class ProtocolHandlerRegistry {
private:
	using ProtocolHandlers = core::Map<ProtocolId, ProtocolHandler *>;
	ProtocolHandlers _registry{256};

public:
	virtual ~ProtocolHandlerRegistry() {
		shutdown();
	}

	void shutdown() {
		_registry.clear();
	}

	inline void registerHandler(const ProtocolId &type, ProtocolHandler *handler) {
		_registry.put(type, handler);
	}

	inline ProtocolHandler *getHandler(const ProtocolMessage &msg) {
		ProtocolHandlers::iterator i = _registry.find(msg.getId());
		if (i != _registry.end())
			return i->second;

		return nullptr;
	}
};


} // namespace voxedit

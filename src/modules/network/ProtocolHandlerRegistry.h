/**
 * @file
 */
#pragma once

#include "ProtocolHandler.h"
#include "core/collection/Map.h"
#include "network/ProtocolMessage.h"

namespace network {

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
		return getHandler(msg.getId());
	}

	inline ProtocolHandler *getHandler(ProtocolId id) {
		ProtocolHandlers::iterator i = _registry.find(id);
		if (i != _registry.end())
			return i->second;

		return nullptr;
	}
};

} // namespace network

/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"
#include "core/collection/Map.h"

namespace ai {

class ProtocolHandlerRegistry {
private:
	typedef core::Map<ProtocolId, IProtocolHandler*> ProtocolHandlers;
	ProtocolHandlers _registry;

	ProtocolHandlerRegistry() {
	}

public:
	static ProtocolHandlerRegistry& get() {
		static ProtocolHandlerRegistry _instance;
		return _instance;
	}

	virtual ~ProtocolHandlerRegistry() {
		_registry.clear();
	}

	inline void registerHandler(const ProtocolId& type, IProtocolHandler* handler) {
		_registry.put(type, handler);
	}

	inline IProtocolHandler* getHandler(const IProtocolMessage& msg) {
		ProtocolHandlers::iterator i = _registry.find(msg.getId());
		if (i != _registry.end())
			return i->second;

		return nullptr;
	}
};

}

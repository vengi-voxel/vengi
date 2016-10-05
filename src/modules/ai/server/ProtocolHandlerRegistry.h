/**
 * @file
 */
#pragma once

#include <unordered_map>
#include "IProtocolHandler.h"

namespace ai {

class ProtocolHandlerRegistry {
private:
	typedef std::unordered_map<ProtocolId, IProtocolHandler*> ProtocolHandlers;
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
		_registry.insert(std::make_pair(type, handler));
	}

	inline IProtocolHandler* getHandler(const IProtocolMessage& msg) {
		ProtocolHandlers::iterator i = _registry.find(msg.getId());
		if (i != _registry.end())
			return i->second;

		return nullptr;
	}
};

}

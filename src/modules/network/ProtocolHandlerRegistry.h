/**
 * @file
 */

#pragma once

#include <memory>
#include <unordered_map>
#include "IProtocolHandler.h"

namespace network {

class ProtocolHandlerRegistry {
private:
	typedef std::unordered_map<const char*, ProtocolHandlerPtr> ProtocolHandlers;
	ProtocolHandlers _registry;

public:
	ProtocolHandlerRegistry();
	~ProtocolHandlerRegistry();

	void shutdown();

	ProtocolHandlerPtr getHandler(const char* type);

	inline void registerHandler(const char* type, const ProtocolHandlerPtr& handler) {
		_registry.insert(std::make_pair(type, handler));
	}
};

typedef std::shared_ptr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

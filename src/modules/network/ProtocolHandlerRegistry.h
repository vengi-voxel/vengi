/**
 * @file
 */

#pragma once

#include "IProtocolHandler.h"
#include "core/Log.h"
#include "core/Enum.h"
#include "core/SharedPtr.h"
#include "core/collection/Map.h"

namespace network {

class ProtocolHandlerRegistry {
private:
	typedef core::Map<uint32_t, ProtocolHandlerPtr> ProtocolHandlers;
	ProtocolHandlers _registry;

public:
	ProtocolHandlerRegistry();
	~ProtocolHandlerRegistry();

	void shutdown();

	template <typename ENUM>
	ProtocolHandlerPtr getHandler(ENUM type) {
		ProtocolHandlers::iterator i = _registry.find(core::enumVal(type));
		if (i != _registry.end()) {
			return i->second;
		}

		Log::error("Failed to get protocol handler for %u", core::enumVal(type));
		return ProtocolHandlerPtr();
	}

	template <typename ENUM>
	inline void registerHandler(const ENUM type, const ProtocolHandlerPtr &handler) {
		_registry.put(core::enumVal(type), handler);
	}
};

typedef core::SharedPtr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

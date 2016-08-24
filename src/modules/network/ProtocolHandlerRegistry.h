/**
 * @file
 */

#pragma once

#include <memory>
#include <unordered_map>
#include "IProtocolHandler.h"
#include "core/Log.h"

namespace network {

class ProtocolHandlerRegistry {
private:
	sauce::shared_ptr<sauce::Injector> _injector;
public:
	ProtocolHandlerRegistry(sauce::shared_ptr<sauce::Injector> injector) :
			_injector(injector) {
	}

	inline ProtocolHandlerPtr getHandler(const char* type) {
		const ProtocolHandlerPtr& ptr = _injector->get<IProtocolHandler>(type);
		::Log::debug("Trying to get handler for %s (%p)", type, ptr.get());
		return ptr;
	}
};

typedef std::shared_ptr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

#pragma once

#include <memory>
#include <unordered_map>
#include "IProtocolHandler.h"

namespace network {

class ProtocolHandlerRegistry {
private:
	sauce::shared_ptr<sauce::Injector> _injector;
public:
	ProtocolHandlerRegistry(sauce::shared_ptr<sauce::Injector> injector) :
			_injector(injector) {
	}

	inline ProtocolHandlerPtr getHandler(int type) {
		return _injector->get<IProtocolHandler>(std::to_string(type));
	}
};

typedef std::shared_ptr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

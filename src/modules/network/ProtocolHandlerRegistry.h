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
	sauce::shared_ptr<sauce::Injector> _injector;
public:
	ProtocolHandlerRegistry(sauce::shared_ptr<sauce::Injector> injector);

	ProtocolHandlerPtr getHandler(const char* type);
};

typedef std::shared_ptr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

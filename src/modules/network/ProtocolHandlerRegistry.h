/**
 * @file
 */

#pragma once

#include <memory>
#include <unordered_map>
#include "IProtocolHandler.h"
#include "core/DependencyInjection.h"

namespace network {

class ProtocolHandlerRegistry {
private:
	std::shared_ptr<di::Injector> _injector;
public:
	ProtocolHandlerRegistry(std::shared_ptr<di::Injector> injector);

	ProtocolHandlerPtr getHandler(const char* type);
};

typedef std::shared_ptr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

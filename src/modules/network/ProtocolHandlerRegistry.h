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
#ifdef DI_SAUCE
	std::shared_ptr<di::Injector> _injector;
#endif
public:
#ifdef DI_SAUCE
	ProtocolHandlerRegistry(std::shared_ptr<di::Injector> injector);
#else
	ProtocolHandlerRegistry();
#endif

	ProtocolHandlerPtr getHandler(const char* type);
};

typedef std::shared_ptr<ProtocolHandlerRegistry> ProtocolHandlerRegistryPtr;

}

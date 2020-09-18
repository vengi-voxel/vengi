/**
 * @file
 */

#include "ProtocolHandlerRegistry.h"
#include "core/Log.h"

namespace network {

ProtocolHandlerRegistry::ProtocolHandlerRegistry() {
}

ProtocolHandlerRegistry::~ProtocolHandlerRegistry() {
	shutdown();
}

void ProtocolHandlerRegistry::shutdown() {
	_registry.clear();
}

ProtocolHandlerPtr ProtocolHandlerRegistry::getHandler(const char* type) {
	ProtocolHandlers::iterator i = _registry.find(type);
	if (i != _registry.end()) {
		return i->second;
	}

	::Log::error("Failed to get protocol handler for %s", type);
	return ProtocolHandlerPtr();
}

}

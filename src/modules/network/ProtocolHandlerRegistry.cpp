/**
 * @file
 */

#include "ProtocolHandlerRegistry.h"

namespace network {

ProtocolHandlerRegistry::ProtocolHandlerRegistry() {
}

ProtocolHandlerRegistry::~ProtocolHandlerRegistry() {
	shutdown();
}

void ProtocolHandlerRegistry::shutdown() {
	_registry.clear();
}

}

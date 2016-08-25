#include "ProtocolHandlerRegistry.h"
#include "core/Log.h"

namespace network {

ProtocolHandlerRegistry::ProtocolHandlerRegistry(std::shared_ptr<di::Injector> injector) :
		_injector(injector) {
}

ProtocolHandlerPtr ProtocolHandlerRegistry::getHandler(const char* type) {
	const std::string typeName(type);
	const ProtocolHandlerPtr& ptr = _injector->get<IProtocolHandler>(typeName);
	::Log::debug("Trying to get handler for %s (%p)", type, ptr.get());
	return ptr;
}

}

/**
 * @file
 */

#pragma once

#include "core/AbstractModule.h"
#include "Network.h"
#include "MessageSender.h"

class NetworkModule: public core::AbstractModule {
protected:
	template<typename Ctor>
	inline void _bindHandler(const std::string& typeName) const {
#ifdef DI_SAUCE
		bind<network::IProtocolHandler>().named(typeName).in<di::SingletonScope>().to<Ctor>();
#endif
#ifdef DI_BOOST
#endif
	}

	void configure() const {
#ifdef DI_SAUCE
		bind<network::ProtocolHandlerRegistry>().in<di::SingletonScope>().to<network::ProtocolHandlerRegistry(di::Injector &)>();
		bind<network::Network>().in<di::SingletonScope>().to<network::Network(network::ProtocolHandlerRegistry &, core::EventBus& eventBus)>();
		bind<network::MessageSender>().in<di::SingletonScope>().to<network::MessageSender(network::Network &)>();
#endif
#ifdef DI_BOOST
		bindSingleton<network::ProtocolHandlerRegistry>();
		bindSingleton<network::Network>();
		bindSingleton<network::MessageSender>();
#endif

		configureHandlers();
	}

	virtual void configureHandlers() const = 0;
};

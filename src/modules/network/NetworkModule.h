/**
 * @file
 */

#pragma once

#include "Network.h"
#include "MessageSender.h"
#include "core/EventBus.h"
#define configureHandler(type, handler) bind<network::IProtocolHandler>().named(type).to<handler>();

class NetworkModule: public sauce::AbstractModule {
	void configure() const {
		bind<network::ProtocolHandlerRegistry>().in<sauce::SingletonScope>().to<network::ProtocolHandlerRegistry(sauce::Injector &)>();
		bind<network::Network>().in<sauce::SingletonScope>().to<network::Network(network::ProtocolHandlerRegistry &, core::EventBus& eventBus)>();
		bind<network::MessageSender>().in<sauce::SingletonScope>().to<network::MessageSender(network::Network &)>();

		configureHandlers();
	}

	virtual void configureHandlers() const = 0;
};

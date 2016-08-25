/**
 * @file
 */

#pragma once

#include "core/AbstractModule.h"
#include "Network.h"
#include "MessageSender.h"

class NetworkModule: public core::AbstractModule {
	void configure() const {
		bind<network::ProtocolHandlerRegistry>().in<sauce::SingletonScope>().to<network::ProtocolHandlerRegistry(sauce::Injector &)>();
		bind<network::Network>().in<sauce::SingletonScope>().to<network::Network(network::ProtocolHandlerRegistry &, core::EventBus& eventBus)>();
		bind<network::MessageSender>().in<sauce::SingletonScope>().to<network::MessageSender(network::Network &)>();

		configureHandlers();
	}

	virtual void configureHandlers() const = 0;
};

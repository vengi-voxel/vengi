/**
 * @file
 */

#pragma once

#include "core/AbstractModule.h"
#include "Network.h"
#include "MessageSender.h"

class NetworkModule: public core::AbstractModule {
	void configure() const {
		bind<network::ProtocolHandlerRegistry>().in<di::SingletonScope>().to<network::ProtocolHandlerRegistry(di::Injector &)>();
		bind<network::Network>().in<di::SingletonScope>().to<network::Network(network::ProtocolHandlerRegistry &, core::EventBus& eventBus)>();
		bind<network::MessageSender>().in<di::SingletonScope>().to<network::MessageSender(network::Network &)>();

		configureHandlers();
	}

	virtual void configureHandlers() const = 0;
};

/**
 * @file
 */

#pragma once

#include "network/NetworkModule.h"
#include "ClientMessages_generated.h"

#include "UserConnectHandler.h"
#include "UserConnectedHandler.h"
#include "UserDisconnectHandler.h"
#include "AttackHandler.h"
#include "MoveHandler.h"

namespace backend {

class ServerNetworkModule: public NetworkModule {
	template<typename Ctor>
	inline void bindHandler(network::messages::client::Type type) const {
		bind<network::IProtocolHandler>().named(network::messages::client::EnumNameType(type)).to<Ctor>();
	}

	void configureHandlers() const override {
		bindHandler<UserConnectHandler(network::Network &, backend::EntityStorage &, voxel::World &)>(network::messages::client::Type::UserConnect);
		bindHandler<UserConnectedHandler>(network::messages::client::Type::UserConnected);
		bindHandler<UserDisconnectHandler>(network::messages::client::Type::UserDisconnect);
		bindHandler<AttackHandler>(network::messages::client::Type::Attack);
		bindHandler<MoveHandler>(network::messages::client::Type::Move);
	}
};

}

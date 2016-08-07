/**
 * @file
 */

#pragma once

#include "../Client.h"
#include "network/NetworkModule.h"
#include "ServerMessages_generated.h"
#include "SeedHandler.h"
#include "AuthFailedHandler.h"
#include "EntityRemoveHandler.h"
#include "EntitySpawnHandler.h"
#include "EntityUpdateHandler.h"
#include "UserSpawnHandler.h"

class ClientNetworkModule: public NetworkModule {
	template<typename Ctor>
	inline void bindHandler(network::messages::server::Type type) const {
		bind<network::IProtocolHandler>().named(network::messages::server::EnumNameType(type)).to<Ctor>();
	}

	void configureHandlers() const override {
		bindHandler<EntitySpawnHandler>(network::messages::server::Type::EntitySpawn);
		bindHandler<EntityRemoveHandler>(network::messages::server::Type::EntityRemove);
		bindHandler<EntityUpdateHandler>(network::messages::server::Type::EntityUpdate);
		bindHandler<UserSpawnHandler>(network::messages::server::Type::UserSpawn);
		bindHandler<AuthFailedHandler>(network::messages::server::Type::AuthFailed);
		bindHandler<SeedHandler(voxel::World &)>(network::messages::server::Type::Seed);
	}
};

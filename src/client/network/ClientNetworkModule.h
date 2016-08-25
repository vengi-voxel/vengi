/**
 * @file
 */

#pragma once

#include "../Client.h"
#include "network/NetworkModule.h"
#include "ServerMessages_generated.h"
#include "AttribUpdateHandler.h"
#include "SeedHandler.h"
#include "AuthFailedHandler.h"
#include "EntityRemoveHandler.h"
#include "EntitySpawnHandler.h"
#include "EntityUpdateHandler.h"
#include "UserSpawnHandler.h"

class ClientNetworkModule: public NetworkModule {
	template<typename Ctor>
	inline void bindHandler(network::ServerMsgType type) const {
		const std::string typeName(network::EnumNameServerMsgType(type));
		bind<network::IProtocolHandler>().named(typeName).in<di::SingletonScope>().to<Ctor>();
	}

	void configureHandlers() const override {
		bindHandler<AttribUpdateHandler>(network::ServerMsgType::AttribUpdate);
		bindHandler<EntitySpawnHandler>(network::ServerMsgType::EntitySpawn);
		bindHandler<EntityRemoveHandler>(network::ServerMsgType::EntityRemove);
		bindHandler<EntityUpdateHandler>(network::ServerMsgType::EntityUpdate);
		bindHandler<UserSpawnHandler>(network::ServerMsgType::UserSpawn);
		bindHandler<AuthFailedHandler>(network::ServerMsgType::AuthFailed);
		bindHandler<SeedHandler(voxel::World &)>(network::ServerMsgType::Seed);
	}
};

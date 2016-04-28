#pragma once

#include "../Client.h"
#include "network/NetworkModule.h"
#include "network/messages/ServerMessages_generated.h"
#include "SeedHandler.h"
#include "AuthFailedHandler.h"
#include "EntityRemoveHandler.h"
#include "EntityUpdateHandler.h"
#include "UserSpawnHandler.h"
#include "NpcSpawnHandler.h"

class ClientNetworkModule: public NetworkModule {
	void configureHandlers() const override {
		configureHandler(Type_NpcSpawn, NpcSpawnHandler);
		configureHandler(Type_EntityRemove, EntityRemoveHandler);
		configureHandler(Type_EntityUpdate, EntityUpdateHandler);
		configureHandler(Type_UserSpawn, UserSpawnHandler);
		configureHandler(Type_Seed, SeedHandler(voxel::World &));
		configureHandler(Type_AuthFailed, AuthFailedHandler);
	}
};

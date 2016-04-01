#pragma once

#include "../Client.h"
#include "network/NetworkModule.h"
#include "network/messages/ServerMessages.h"
#include "SeedHandler.h"
#include "AuthFailedHandler.h"
#include "UserSpawnHandler.h"
#include "UserUpdateHandler.h"
#include "NpcSpawnHandler.h"
#include "NpcUpdateHandler.h"
#include "NpcRemoveHandler.h"

class ClientNetworkModule: public NetworkModule {
	void configureHandlers() const override {
		configureHandler(Type_NpcSpawn, NpcSpawnHandler);
		configureHandler(Type_NpcRemove, NpcRemoveHandler);
		configureHandler(Type_NpcUpdate, NpcUpdateHandler);
		configureHandler(Type_UserSpawn, UserSpawnHandler);
		configureHandler(Type_UserUpdate, UserUpdateHandler);
		configureHandler(Type_Seed, SeedHandler(voxel::World &));
		configureHandler(Type_AuthFailed, AuthFailedHandler);
	}
};
